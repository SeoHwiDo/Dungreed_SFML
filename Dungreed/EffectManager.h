#pragma once

#include <functional>
#include <unordered_set>
#include <vector>

#include "Effect.h"

class ObjectPoolingManager;

/// 이펙트의 발동 시점, 수명 갱신, 풀 반환, 렌더링을 담당합니다.
/// 객체의 생성·소유·재사용은 ObjectPoolingManager에만 맡깁니다.
class EffectManager {
public:
    static EffectManager& getInstance() {
        static EffectManager instance;
        return instance;
    }

    EffectManager(const EffectManager&) = delete;
    EffectManager& operator=(const EffectManager&) = delete;
    /// 플레이어 근접 공격을 표현하고 실제 타격 범위를 제공하는 SwingFX를 대여합니다.
    void spawnPlayerSwing(ObjectPoolingManager& objectPool,
        const sf::Vector2f& playerPosition, float aimRadian, float damage);
    /// 몬스터 피격 지점에 짧게 재생되는 SlashFX를 대여합니다.
    void spawnHitSlash(ObjectPoolingManager& objectPool,
        const sf::Vector2f& hitPosition, float rotationRadian);
    /// 몬스터가 등장하는 위치에 반투명 MagicCircleFx를 겹쳐 재생합니다.
    /// 매직서클 중간 프레임이 도달하는 시간을 반환합니다.
    float spawnMonsterMagicCircle(ObjectPoolingManager& objectPool,
        const sf::Vector2f& monsterPosition);
    /// 전투방 클리어 보물상자가 나타나는 위치에 불투명한 마법진을 재생합니다.
    void spawnRewardChestMagicCircle(ObjectPoolingManager& objectPool,
        const sf::Vector2f& chestPosition);
    /// 활성 이펙트를 갱신하고 끝난 이펙트를 풀에 반환합니다.
    void update(float dt, ObjectPoolingManager& objectPool);
    /// 활성 공격 이펙트를 순회해 CombatManager가 피해를 확정할 수 있게 합니다.
    void forEachActiveAttackEffect(const ObjectPoolingManager& objectPool,
        const std::function<void(Effect&)>& operation) const;
    /// 활성 이펙트를 모든 액터 위에 렌더링합니다.
    void render(sf::RenderWindow& window, const ObjectPoolingManager& objectPool) const;
    /// 방 전환 때 남아 있는 이펙트를 즉시 풀로 돌려보냅니다.
    void clear(ObjectPoolingManager& objectPool);
private:
    EffectManager() {
        m_expiredEffects.reserve(40);
        m_activeEffects.reserve(40);
    }
    ~EffectManager() = default;
    std::vector<Effect*> m_expiredEffects;
    std::vector<Effect*> m_activeEffects;
};
