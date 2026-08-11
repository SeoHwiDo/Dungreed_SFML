#pragma once

#include <functional>
#include <memory>
#include <queue>
#include <string>
#include <vector>

#include "Monster.h"
#include "Projectile.h"
#include "Effect.h"

class GameDataManager;

/// 몬스터와 투사체의 생성·소유·재사용·반환만 담당하는 중앙 오브젝트 풀 관리자입니다.
class ObjectPoolingManager {
public:
    static ObjectPoolingManager& getInstance() {
        static ObjectPoolingManager instance;
        return instance;
    }

    ObjectPoolingManager(const ObjectPoolingManager&) = delete;
    ObjectPoolingManager& operator=(const ObjectPoolingManager&) = delete;
    /// 지정 수만큼 몬스터를 미리 생성해 비활성 슬롯으로 넣습니다. 이후 요청은 가장 먼저 비활성화된 슬롯부터 재사용합니다.
    void prewarmMonsters(std::size_t count, const std::string& type,
        Actor::Status status = { kDefaultMaxHp, kDefaultMaxHp, kDefaultPower, kDefaultDex },
        const std::string& atlasKey = "Monster",
        MonsterBehaviorConfig behavior = {});
    /// 지정 수만큼 기본 비활성 투사체를 미리 생성해 전투 중 동적 할당을 줄입니다.
    void prewarmProjectiles(std::size_t count);
    /// 화면 효과를 미리 생성해 비활성 슬롯으로 준비합니다.
    void prewarmEffects(std::size_t count);
    void prewarmFromGameData(const GameDataManager& gameData,
        float reserveRatio = 0.10f);

    /// 비활성 몬스터 슬롯을 재사용하거나 새 슬롯을 생성해 활성 몬스터를 반환합니다.
    Monster* acquireMonster(const std::string& type,
        Actor::Status status = { kDefaultMaxHp, kDefaultMaxHp, kDefaultPower, kDefaultDex },
        const std::string& atlasKey = "Monster",
        MonsterBehaviorConfig behavior = {});
    /// 몬스터를 삭제하지 않고 비활성 슬롯으로 반환합니다.
    void releaseMonster(Monster* monster);
    /// 현재 활성화된 모든 몬스터에 대해 상호작용 매니저가 연산할 수 있도록 순회합니다.
    void forEachActiveMonster(const std::function<void(Monster&)>& operation);

    /// 비활성 투사체 슬롯을 재사용하거나 새 슬롯을 생성합니다.
    Projectile* acquireProjectile(const ProjectileSpawnRequest& request);
    /// 투사체를 삭제하지 않고 비활성 슬롯으로 반환합니다.
    void releaseProjectile(Projectile* projectile);
    /// 현재 활성 투사체를 순회합니다. 충돌 연산은 호출자 매니저가 수행합니다.
    void forEachActiveProjectile(const std::function<void(Projectile&)>& operation);
    /// 비활성 효과 슬롯을 재사용하거나 새 슬롯을 생성합니다.
    Effect* acquireEffect(const EffectSpawnRequest& request);
    /// 효과를 삭제하지 않고 비활성 슬롯으로 반환합니다.
    void releaseEffect(Effect* effect);
    /// 활성 효과를 순회합니다. 이펙트 매니저가 갱신·렌더링·반환을 관제합니다.
    void forEachActiveEffect(const std::function<void(Effect&)>& operation) const;
    /// 모든 활성 객체를 창에 렌더링합니다.
    void render(sf::RenderWindow& window) const;
    /// 타일맵 뒤로 들어가야 하는 매립형 투사체 조각을 먼저 렌더링합니다.
    void renderBehindTiles(sf::RenderWindow& window) const;

private:
    ObjectPoolingManager() = default;
    ~ObjectPoolingManager() = default;
    struct MonsterSlot {
        std::unique_ptr<Monster> object;
        bool active = false;
    };
    struct ProjectileSlot {
        std::unique_ptr<Projectile> object;
        bool active = false;
    };
    struct EffectSlot {
        std::unique_ptr<Effect> object;
        bool active = false;
    };

    /// 비활성 슬롯의 반환 순서를 보관합니다. 값이 작을수록 먼저 제공되는 최소 힙입니다.
    struct InactiveSlot {
        std::size_t index = 0;
        std::size_t availableOrder = 0;
    };
    struct InactiveSlotCompare {
        bool operator()(const InactiveSlot& left, const InactiveSlot& right) const {
            return left.availableOrder > right.availableOrder;
        }
    };

    /// 슬롯을 비활성 우선순위 큐에 넣어 FIFO 재사용 순서를 부여합니다.
    void enqueueInactiveMonster(std::size_t index);
    /// 슬롯을 비활성 우선순위 큐에 넣어 FIFO 재사용 순서를 부여합니다.
    void enqueueInactiveProjectile(std::size_t index);
    /// 효과 슬롯을 비활성 우선순위 큐에 넣어 FIFO 대여 순서를 유지합니다.
    void enqueueInactiveEffect(std::size_t index);
    /// 큐에서 가장 오래 비활성 상태였던 유효한 몬스터 슬롯을 꺼냅니다.
    MonsterSlot* dequeueInactiveMonster();
    /// 큐에서 가장 오래 비활성 상태였던 유효한 투사체 슬롯을 꺼냅니다.
    ProjectileSlot* dequeueInactiveProjectile();
    /// 큐에서 가장 오래된 유효 비활성 효과 슬롯을 꺼냅니다.
    EffectSlot* dequeueInactiveEffect();

    std::vector<MonsterSlot> m_monsters;
    std::vector<ProjectileSlot> m_projectiles;
    std::vector<EffectSlot> m_effects;
    std::priority_queue<InactiveSlot, std::vector<InactiveSlot>, InactiveSlotCompare>
        m_inactiveMonsterSlots;
    std::priority_queue<InactiveSlot, std::vector<InactiveSlot>, InactiveSlotCompare>
        m_inactiveProjectileSlots;
    std::priority_queue<InactiveSlot, std::vector<InactiveSlot>, InactiveSlotCompare>
        m_inactiveEffectSlots;
    std::size_t m_nextAvailableOrder = 0;
};
