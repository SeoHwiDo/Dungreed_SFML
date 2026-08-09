#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <optional>
#include <vector>
#include <unordered_set>
#include <cstdint>
#include "ResourceManager.h"

class Actor; // 전방 선언
using EntityId = std::uint32_t;

enum class WeaponType {
    Melee,
    Ranged
};

enum class ProjectileType {
    Arrow,
    Fireball,
    Bullet,
    BabyBatBullet,
    BansheeBullet
};

enum class ProjectileTarget {
    Player,
    Monster
};

/// 원거리 장비가 생성할 투사체의 공통 설정입니다.
struct ProjectileConfig {
    ProjectileType type = ProjectileType::Arrow;
    ProjectileTarget target = ProjectileTarget::Monster;
    float speed = 400.f;
    float damage = 10.f;
    unsigned int count = 1;
    float spreadRadian = 0.f;
    float lifetime = 3.f;
};

/// 장비가 풀 매니저에 전달하는 투사체 생성 요청입니다. 실제 Projectile 객체는 장비가 만들지 않습니다.
struct ProjectileSpawnRequest {
    ProjectileType type = ProjectileType::Arrow;
    ProjectileTarget target = ProjectileTarget::Monster;
    sf::Vector2f position;
    sf::Vector2f direction{ 1.f, 0.f };
    float speed = 400.f;
    float damage = 10.f;
    float lifetime = 3.f;
};

struct EquipStat {
    float damage = 10.f;      // 무기 공격력
    float attackSpeed = 1.f;  // 공격 속도
    float range = 50.f;       // 공격 사거리
    WeaponType type = WeaponType::Melee;
    std::optional<ProjectileConfig> projectile;
};

class Equip {
public:
    /// 이름과 능력치만 가진 장비를 생성합니다. 표시 전에 init으로 아틀라스 프레임을 연결해야 합니다.
    Equip(const std::string& name, EquipStat stat);
    virtual ~Equip() = default;

    /// JSON 피벗을 포함한 아틀라스 프레임을 읽어 무기 스프라이트를 초기화합니다.
    void init(const std::string& atlasKey, const std::string& frameName);

    /// 장비를 소유한 액터를 기록합니다. 현재는 소유 관계 보관용이며 위치 갱신 인자는 별도로 받습니다.
    inline void setOwner(Actor* owner) { m_owner = owner; }
    /// 무기 공격력·속도·사거리 정보를 값으로 반환합니다.
    inline EquipStat getStat() const { return m_stat; }
    /// UI나 로그에 쓸 장비 이름을 반환합니다.
    inline const std::string& getName() const { return m_name; }

    /// 소유자 몸통 위치와 조준 라디안으로 무기 위치·좌우 반전·스윙 회전을 갱신합니다.
    virtual void update(float dt, const sf::Vector2f& ownerPos, float aimRadian);

    /// 초기화된 무기 스프라이트를 창에 그립니다.
    virtual void render(sf::RenderWindow& window);

    /// 공격 중일 때만 월드 좌표 히트박스를 반환합니다. 비공격/미초기화 상태면 nullopt입니다.
    std::optional<sf::FloatRect> getAttackHitbox() const;

    /// 새 스윙을 시작하고 같은 대상에 대한 피해 처리 여부를 초기화합니다.
    void attack();
    /// 스윙 애니메이션이 진행 중인지 반환합니다.
    inline bool isAttacking() const { return m_isAttacking; }
    /// 한 스윙에서 처음 호출될 때만 true를 반환해 중복 피해를 막습니다.
    bool consumeHit(EntityId targetId);

    /// 원거리 공격 요청을 장비 설정에 맞춰 생성하고, 같은 공격 요청의 중복 생성을 막습니다.
    std::vector<ProjectileSpawnRequest> consumeProjectileRequests();

protected:
    std::string m_name;
    EquipStat m_stat;
    std::optional<sf::Sprite> m_sprite;
    Actor* m_owner = nullptr; // 무기 소유자

    // 공격 애니메이션 및 상태 처리를 위한 변수
    bool m_isAttacking = false;
    std::unordered_set<EntityId> m_hitTargets;
    bool m_isSwung = false;   // 추가: 현재 무기가 꺾인 상태(토글)인지 확인
    float m_attackTimer = 0.f;
    float m_attackDuration = 0.5f;
    bool m_projectileRequestPending = false;
    sf::Vector2f m_lastOwnerPosition;
    float m_lastAimRadian = 0.f;
};
