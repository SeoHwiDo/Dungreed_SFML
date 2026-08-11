#include "CombatManager.h"

#include "Player.h"
#include "Boss.h"
#include "Monster.h"
#include "TileMap.h"
#include "Collision.h"
#include "EffectManager.h"
#include <algorithm>
#include <cmath>
#include <vector>

std::unordered_set<EntityId> CombatManager::resolvePlayerAttack(
    Player& player, ObjectPoolingManager& objectPool, EffectManager& effectManager,
    Boss* boss) const {
    std::unordered_set<EntityId> hitMonsters;
    const auto weapon = player.getEquipment();
    if (!weapon) {
        return hitMonsters;
    }

    // 근접 무기는 공격 중인 스프라이트 경계를 즉시 검사합니다.
    if (weapon->getStat().type == WeaponType::Melee &&
        weapon->consumeMeleeSwingStarted()) {
        effectManager.spawnPlayerSwing(objectPool, player.getBodyCenterPosition(),
            weapon->getAimRadian(), weapon->getStat().damage);
    }
    // SwingFX와 실제 무기 스프라이트 중 하나라도 닿으면 적중으로 처리합니다.
    // 대상별 1회 타격 제한은 SwingFX가 관리합니다.
    const auto weaponAttackBox = weapon->getAttackHitbox();
    effectManager.forEachActiveAttackEffect(objectPool, [&](Effect& effect) {
        const auto attackBox = effect.getAttackHitbox();
        if (!attackBox) {
            return;
        }
        objectPool.forEachActiveMonster([&](Monster& monster) {
            std::optional<sf::Vector2f> hitPosition;
            if (!monster.dead()) {
                hitPosition = monster.getCollision().checkHit(*attackBox);
                if (!hitPosition && weaponAttackBox) {
                    hitPosition = monster.getCollision().checkHit(*weaponAttackBox);
                }
            }
            if (hitPosition && effect.consumeHit(monster.getId())) {
                monster.takeDamage(effect.getDamage(), effect.getAttackerPosition());
                hitMonsters.insert(monster.getId());
                effectManager.spawnHitSlash(objectPool, *hitPosition,
                    effect.getRotationRadian());
            }
        });
        if (boss && !boss->dead()) {
            auto hitPosition = boss->getCollision().checkHit(*attackBox);
            if (!hitPosition && weaponAttackBox) {
                hitPosition = boss->getCollision().checkHit(*weaponAttackBox);
            }
            if (hitPosition && effect.consumeHit(boss->getId())) {
                boss->takeDamage(effect.getDamage(), effect.getAttackerPosition(), 0.f);
                effectManager.spawnHitSlash(objectPool, *hitPosition,
                    effect.getRotationRadian());
            }
        }
    });

    // 원거리 무기는 장비의 설정을 요청으로 변환하고, 실제 객체 생성은 풀에 맡깁니다.
    for (const ProjectileSpawnRequest& request : weapon->consumeProjectileRequests()) {
        objectPool.acquireProjectile(request);
    }
    return hitMonsters;
}

void CombatManager::resolveMonsterAttacks(float dt, Player& player,
    ObjectPoolingManager& objectPool,
    const TileMap& tileMap,
    const std::unordered_set<EntityId>& playerHitMonsters) const {
    if (player.dead() || player.isDashing()) {
        return;
    }

    sf::FloatRect playerBounds = player.getGlobalBounds();
    objectPool.forEachActiveMonster([&](Monster& monster) {
        if (monster.dead()) {
            return;
        }

        // 같은 프레임에 플레이어에게 맞은 몬스터의 공격은 돌진을 포함해 무효화합니다.
        if (playerHitMonsters.find(monster.getId()) != playerHitMonsters.end()) {
            return;
        }

        if (monster.getAttackPattern() == MonsterAttackPattern::ChargeCombo &&
            monster.state == MonsterState::Charge) {
            const bool isChargeContact = monster.isChargeImpactActive() &&
                monster.getCollision().checkHit(playerBounds).has_value();
            if (isChargeContact) {
                if (monster.consumeChargeImpact()) {
                    player.applyStun(monster.getChargeStunDuration());
                }
                // 충돌한 뒤에는 이번 프레임 미노타우르스가 실제 이동한 거리만큼 함께 이동합니다.
                const sf::FloatRect& previousBounds = monster.getPreviousGlobalBounds();
                const sf::FloatRect currentBounds = monster.getGlobalBounds();
                player.move(currentBounds.position.x - previousBounds.position.x,
                    currentBounds.position.y - previousBounds.position.y);
                Collision::resolveMapCollision(player, tileMap,
                    player.ignoresOneWayPlatforms());
                playerBounds = player.getGlobalBounds();
            }
            return;
        }

        if (playerHitMonsters.find(monster.getId()) != playerHitMonsters.end()) {
            // 동일 프레임에 플레이어가 맞힌 몬스터의 공격만 무효화합니다.
            return;
        }

        if (monster.state != MonsterState::Attack) {
            return;
        }

        const auto weapon = monster.getEquipment();
        if (!weapon) {
            return;
        }

        const bool isMelee = weapon->getStat().type == WeaponType::Melee;
        // 근접 공격은 원형 사거리가 아니라 몬스터가 바라보는 전면부와의 실제 충돌만 허용합니다.
        if (isMelee && !monster.isTargetInFrontContact(playerBounds)) {
            return;
        }
        const sf::Vector2f playerCenter = player.getBodyCenterPosition();
        if (!isMelee && !monster.isTargetInAttackRange(playerCenter)) {
            return;
        }
        if (!monster.consumeAttackAction()) {
            return;
        }

        const sf::Vector2f origin = monster.getBodyCenterPosition();
        const sf::Vector2f toPlayer = player.getBodyCenterPosition() - origin;
        const float aim = std::atan2(toPlayer.y, toPlayer.x);
        weapon->update(dt, origin, aim);
        weapon->attack();

        if (!isMelee) {
            for (const ProjectileSpawnRequest& request : weapon->consumeProjectileRequests()) {
                objectPool.acquireProjectile(request);
            }
            return;
        }

        player.takeDamage(weapon->getStat().damage, origin);
    });
}

std::unordered_set<EntityId> CombatManager::updateProjectiles(
    float dt, Player& player, ObjectPoolingManager& objectPool,
    const TileMap& tileMap, ProjectileTarget target, Boss* boss) const {
    std::unordered_set<EntityId> hitMonsters;
    std::vector<Projectile*> toRelease;

    objectPool.forEachActiveProjectile([&](Projectile& projectile) {
        if (!projectile.isActive() || projectile.getTarget() != target) {
            return;
        }
        projectile.update(dt);
        if (!projectile.isActive()) {
            toRelease.push_back(&projectile);
            return;
        }
        // Trail은 시각 효과만 남긴 상태이므로 이동·벽 충돌·피해 판정을 모두 건너뜁니다.
        if (projectile.isPlayingReturnTrail()) {
            return;
        }
        if (projectile.isEmbedded()) {
            return;
        }
        if (projectile.getType() == ProjectileType::BossSword &&
            !projectile.isDamageActive()) {
            return;
        }
        if (Collision::resolveProjectileMapCollision(projectile, tileMap)) {
            if (projectile.getType() == ProjectileType::BossSword) {
                const sf::FloatRect swordBounds = projectile.getGlobalBounds();
                const float halfSwordLength =
                    std::max(swordBounds.size.x, swordBounds.size.y) * 0.5f;
                const sf::Vector2f impactPosition = projectile.getPosition() +
                    projectile.getDirection() * halfSwordLength;
                objectPool.acquireEffect({
                    "Projectile", "BossSwordHitFX", impactPosition,
                    projectile.getRotationRadian(), { 1.f, 1.f }, 0.06f,
                    false, false, 0.f, impactPosition, sf::Color::White
                });
                // 칼 중심이 충돌면 안쪽 약 5px에 오도록 넣은 뒤, 타일 뒤에서 렌더링합니다.
                projectile.embedInWall(1.25f, halfSwordLength + 5.f);
            } else {
                toRelease.push_back(&projectile);
            }
            return;
        }

        if (target == ProjectileTarget::Player) {
            if (!player.dead() && !player.isDashing() &&
                projectile.checkHit(player.getGlobalBounds())) {
                // 투사체는 근접 공격의 절반 힘으로만 넉백을 적용합니다.
                player.takeDamage(projectile.getDamage(), projectile.getPosition(), 0.5f);
                toRelease.push_back(&projectile);
            }
            return;
        }

        bool hit = false;
        objectPool.forEachActiveMonster([&](Monster& monster) {
            if (hit) {
                return;
            }
            if (!monster.dead() && projectile.checkHit(monster.getGlobalBounds())) {
                monster.takeDamage(projectile.getDamage(), projectile.getPosition());
                hitMonsters.insert(monster.getId());
                hit = true;
            }
        });
        if (!hit && boss && !boss->dead() &&
            projectile.checkHit(boss->getGlobalBounds())) {
            boss->takeDamage(projectile.getDamage(), projectile.getPosition(), 0.f);
            hitMonsters.insert(boss->getId());
            hit = true;
        }
        if (hit) {
            toRelease.push_back(&projectile);
        }
    });

    for (Projectile* projectile : toRelease) {
        objectPool.releaseProjectile(projectile);
    }
    return hitMonsters;
}
