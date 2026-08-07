#include "CombatManager.h"

#include "Player.h"
#include "Monster.h"
#include "TileMap.h"
#include "Collision.h"
#include <cmath>
#include <vector>

std::unordered_set<EntityId> CombatManager::resolvePlayerAttack(
    Player& player, ObjectPoolingManager& objectPool) const {
    std::unordered_set<EntityId> hitMonsters;
    const auto weapon = player.getEquipment();
    if (!weapon) {
        return hitMonsters;
    }

    // 근접 무기는 공격 중인 스프라이트 경계를 즉시 검사합니다.
    if (const auto attackBox = weapon->getAttackHitbox()) {
        objectPool.forEachActiveMonster([&](Monster& monster) {
            if (!monster.dead() && monster.getCollision().checkHit(*attackBox).has_value()
                && weapon->consumeHit(monster.getId())) {
                monster.takeDamage(weapon->getStat().damage, player.getBodyCenterPosition());
                hitMonsters.insert(monster.getId());
            }
        });
    }

    // 원거리 무기는 장비의 설정을 요청으로 변환하고, 실제 객체 생성은 풀에 맡깁니다.
    for (const ProjectileSpawnRequest& request : weapon->consumeProjectileRequests()) {
        objectPool.acquireProjectile(request);
    }
    return hitMonsters;
}

void CombatManager::resolveMonsterAttacks(float dt, Player& player,
    ObjectPoolingManager& objectPool,
    const std::unordered_set<EntityId>& playerHitMonsters) const {
    const sf::FloatRect playerBounds = player.getGlobalBounds();
    objectPool.forEachActiveMonster([&](Monster& monster) {
        if (monster.dead() || playerHitMonsters.find(monster.getId()) != playerHitMonsters.end()) {
            // 동일 프레임에 플레이어가 맞힌 몬스터의 공격만 무효화합니다.
            return;
        }

        const auto weapon = monster.getEquipment();
        if (!weapon || !monster.consumeAttackCooldown(dt)) {
            return;
        }

        const sf::Vector2f origin = monster.getBodyCenterPosition();
        const sf::Vector2f toPlayer = player.getBodyCenterPosition() - origin;
        const float aim = std::atan2(toPlayer.y, toPlayer.x);
        weapon->update(dt, origin, aim);
        weapon->attack();

        if (weapon->getStat().type == WeaponType::Ranged) {
            for (const ProjectileSpawnRequest& request : weapon->consumeProjectileRequests()) {
                objectPool.acquireProjectile(request);
            }
            monster.beginAttack();
            return;
        }

        if (monster.getCollision().checkHit(playerBounds).has_value()) {
            player.takeDamage(weapon->getStat().damage, origin);
            monster.beginAttack();
        }
    });
}

std::unordered_set<EntityId> CombatManager::updateProjectiles(
    float dt, Player& player, ObjectPoolingManager& objectPool,
    const TileMap& tileMap, ProjectileTarget target) const {
    std::unordered_set<EntityId> hitMonsters;
    std::vector<Projectile*> toRelease;

    objectPool.forEachActiveProjectile([&](Projectile& projectile) {
        if (!projectile.isActive() || projectile.getTarget() != target) {
            return;
        }
        projectile.update(dt);
        if (!projectile.isActive() || Collision::resolveProjectileMapCollision(projectile, tileMap)) {
            toRelease.push_back(&projectile);
            return;
        }

        if (target == ProjectileTarget::Player) {
            if (projectile.checkHit(player.getGlobalBounds())) {
                player.takeDamage(projectile.getDamage(), projectile.getPosition());
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
        if (hit) {
            toRelease.push_back(&projectile);
        }
    });

    for (Projectile* projectile : toRelease) {
        objectPool.releaseProjectile(projectile);
    }
    return hitMonsters;
}
