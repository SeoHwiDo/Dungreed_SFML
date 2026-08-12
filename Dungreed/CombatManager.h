#pragma once

#include "ObjectPoolingManager.h"
#include <unordered_set>
#include <vector>

class Player;
class Boss;
class TileMap;
class EffectManager;

/// 벽 → 플레이어 공격 → 몬스터 공격 → 투사체 순서로 전투 상호작용을 실행합니다.
/// 몬스터·투사체의 생성과 소유는 ObjectPoolingManager에 위임하고, 이 클래스는 판정만 담당합니다.
class CombatManager {
  public:
    static CombatManager &getInstance() {
        static CombatManager instance;
        return instance;
    }

    CombatManager(const CombatManager &) = delete;
    CombatManager &operator=(const CombatManager &) = delete;
    /// 플레이어의 근접 공격과 원거리 생성 요청을 처리하고, 이번 프레임에 맞은 몬스터 ID를 반환합니다.
    std::unordered_set<EntityId> resolvePlayerAttack(Player &player, ObjectPoolingManager &objectPool, EffectManager &effectManager, Boss *boss = nullptr) const;

    /// 몬스터의 근접/원거리 공격을 처리합니다. playerHitMonsters에는 이번 프레임 플레이어가 맞힌 몬스터를 전달합니다.
    void resolveMonsterAttacks(float dt, Player &player, ObjectPoolingManager &objectPool, const TileMap &tileMap, const std::unordered_set<EntityId> &playerHitMonsters) const;

    /// 지정한 대상 그룹의 투사체만 한 번 갱신합니다. 벽 충돌을 가장 먼저 검사합니다.
    std::unordered_set<EntityId> updateProjectiles(float dt, Player &player, ObjectPoolingManager &objectPool, const TileMap &tileMap, ProjectileTarget target, Boss *boss = nullptr) const;

  private:
    CombatManager() { m_projectilesToRelease.reserve(384); }
    ~CombatManager() = default;
    mutable std::vector<Projectile *> m_projectilesToRelease;
};
