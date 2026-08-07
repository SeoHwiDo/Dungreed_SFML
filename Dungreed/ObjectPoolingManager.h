#pragma once

#include <functional>
#include <memory>
#include <queue>
#include <string>
#include <vector>

#include "Monster.h"
#include "Projectile.h"

/// 몬스터와 투사체의 생성·소유·재사용·반환만 담당하는 중앙 오브젝트 풀 관리자입니다.
class ObjectPoolingManager {
public:
    /// 지정 수만큼 몬스터를 미리 생성해 비활성 슬롯으로 넣습니다. 이후 요청은 가장 먼저 비활성화된 슬롯부터 재사용합니다.
    void prewarmMonsters(std::size_t count, const std::string& type,
        Actor::Status status = { MAXHP, MAXHP, POWER, DEX },
        const std::string& atlasKey = "Monster");
    /// 지정 수만큼 기본 비활성 투사체를 미리 생성해 전투 중 동적 할당을 줄입니다.
    void prewarmProjectiles(std::size_t count);

    /// 비활성 몬스터 슬롯을 재사용하거나 새 슬롯을 생성해 활성 몬스터를 반환합니다.
    Monster* acquireMonster(const std::string& type,
        Actor::Status status = { MAXHP, MAXHP, POWER, DEX },
        const std::string& atlasKey = "Monster");
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
    /// 모든 활성 객체를 창에 렌더링합니다.
    void render(sf::RenderWindow& window) const;

private:
    struct MonsterSlot {
        std::unique_ptr<Monster> object;
        bool active = false;
    };
    struct ProjectileSlot {
        std::unique_ptr<Projectile> object;
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
    /// 큐에서 가장 오래 비활성 상태였던 유효한 몬스터 슬롯을 꺼냅니다.
    MonsterSlot* dequeueInactiveMonster();
    /// 큐에서 가장 오래 비활성 상태였던 유효한 투사체 슬롯을 꺼냅니다.
    ProjectileSlot* dequeueInactiveProjectile();

    std::vector<MonsterSlot> m_monsters;
    std::vector<ProjectileSlot> m_projectiles;
    std::priority_queue<InactiveSlot, std::vector<InactiveSlot>, InactiveSlotCompare>
        m_inactiveMonsterSlots;
    std::priority_queue<InactiveSlot, std::vector<InactiveSlot>, InactiveSlotCompare>
        m_inactiveProjectileSlots;
    std::size_t m_nextAvailableOrder = 0;
};
