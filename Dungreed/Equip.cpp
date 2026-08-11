#include "Equip.h"
#include "Actor.h"
#include <algorithm>
#include <cmath>
#include <iostream>

Equip::Equip(const std::string& name, EquipStat stat)
    : m_name(name), m_stat(stat) {}

void Equip::init(const std::string& atlasKey, const std::string& frameName) {
    auto& resMgr = ResourceManager::getInstance();
    const sf::Texture* tex = resMgr.getAtlasTexture(atlasKey);
    // equip_atlas.json 프레임 형식("ShortSword.png")에 맞게 수정


    // 확장자 .png가 없으면 자동으로 붙여주도록 처리
    std::string actualFrameName = frameName;
    if (actualFrameName.find(".png") == std::string::npos) {
        actualFrameName += ".png";
    }
    const sf::IntRect* rect = resMgr.getFrameRect(atlasKey, actualFrameName);
    const auto pivot = resMgr.getFramePivot(atlasKey, actualFrameName);

    if (tex && rect) {
        m_sprite.emplace(*tex);
        m_sprite->setTextureRect(*rect);
        m_sprite->setOrigin(pivot.value_or(sf::Vector2f{
            rect->size.x / 2.f, rect->size.y / 2.f
        }));
    } else {
        std::cerr << "[Equip] 무기 스프라이트를 찾을 수 없습니다: " << actualFrameName << std::endl;
    }
}

void Equip::attack() {
    if (m_stat.type == WeaponType::Ranged) {
        // 원거리 장비는 스윙 상태 대신 한 번의 투사체 생성 요청만 예약합니다.
        m_projectileRequestPending = true;
        return;
    }

    // 이미 공격 중이 아닐 때만 공격 실행
    if (!m_isAttacking) {
        m_isAttacking = true;
        m_meleeSwingStarted = true;
        m_attackTimer = 0.f;
        // stat의 attackSpeed를 반영하여 공격 시간 설정 (기본 0.5초 기준)
        m_attackDuration = 0.1f / (m_stat.attackSpeed > 0.f ? m_stat.attackSpeed : 1.f);
    }
}

bool Equip::consumeMeleeSwingStarted() {
    if (!m_meleeSwingStarted) {
        return false;
    }
    m_meleeSwingStarted = false;
    return true;
}

std::vector<ProjectileSpawnRequest> Equip::consumeProjectileRequests() {
    std::vector<ProjectileSpawnRequest> requests;
    if (!m_projectileRequestPending || !m_stat.projectile) {
        return requests;
    }

    m_projectileRequestPending = false;
    const ProjectileConfig& config = *m_stat.projectile;
    const unsigned int count = std::max(1u, config.count);
    for (unsigned int index = 0; index < count; ++index) {
        const float center = (static_cast<float>(count) - 1.f) * 0.5f;
        const float angle = m_lastAimRadian +
            (static_cast<float>(index) - center) * config.spreadRadian;
        requests.push_back({
            config.type,
            m_stat.type == WeaponType::Ranged,
            config.animationKey,
            config.target,
            m_lastOwnerPosition,
            { std::cos(angle), std::sin(angle) },
            config.speed,
            config.damage,
            config.lifetime
        });
        ProjectileSpawnRequest& request = requests.back();
        request.returnAnimationKey = config.returnAnimationKey;
        request.rotateToDirection = config.rotateToDirection;
        request.rotationOffsetRadian = config.rotationOffsetRadian;
    }
    return requests;
}

void Equip::update(float dt, const sf::Vector2f& ownerPos, float aimRadian) {
    m_lastOwnerPosition = ownerPos;
    m_lastAimRadian = aimRadian;
    if (!m_sprite || m_stat.type == WeaponType::Ranged) return;
    constexpr float kPi = 3.14159265358979323846f;
    constexpr float kHalfPi = kPi / 2.f;

    // 실제 몸통 중앙에서 캐릭터가 바라보는 전방으로 무기를 배치합니다.
    const bool isFacingLeft = std::abs(aimRadian) > kHalfPi;
    const float facingDirection = isFacingLeft ? -1.f : 1.f;
    const float bodyWidth = m_owner ? m_owner->getGlobalBounds().size.x : 0.f;
    const float bodyHeight = m_owner ? m_owner->getGlobalBounds().size.y : 0.f;

    const float forwardOffset = std::max(bodyWidth * 0.35f, 5.f);
    const float upDownOffset = std::max(bodyHeight * 0.25f, 5.f);

    const sf::Vector2f adjustedPos = {
        ownerPos.x + facingDirection * forwardOffset,
        ownerPos.y + upDownOffset
    };
    m_sprite->setPosition(adjustedPos);

    // 기본 무기 이미지가 12시 방향을 향하므로 π/2를 보정합니다.
    // 좌측은 Y축 반전과 반대 보정을 조합해, 각도를 360도 회전시키지 않고
    // 좌우가 대칭인 자세로 표시합니다.
    const float baseRotation = aimRadian + (isFacingLeft ? -kHalfPi : kHalfPi);

    // 현재 대기 각도에서 반대쪽 끝까지 한 번만 스윙합니다.
    // startOffset은 대기 상태의 각도와 같아야 공격 시작 시 반대편으로
    // 되돌아가는(왕복처럼 보이는) 순간 이동이 발생하지 않습니다.
    float angle = 70.f;
    float startOffset = m_isSwung ? angle : -angle;
    float endOffset = m_isSwung ? -angle : angle;
    float swingOffset = m_isSwung ? angle : -angle; // 공격 대기 상태

    if (m_isAttacking) {
        m_attackTimer += dt;

        // 실제 스윙이 진행되는 구간
        if (m_attackTimer <= m_attackDuration) {
            // 0 ~ 1 사이의 진행도에 따라 선형 보간(Lerp) 적용
            float progress = m_attackTimer / m_attackDuration;
            swingOffset = startOffset + (endOffset - startOffset) * progress;
        }
        // 스윙 완료 후 0.15초간 쿨타임을 주어 연타/홀딩 시 즉시 되돌아오는 현상 방지
        else if (m_attackTimer <= m_attackDuration + 0.15f) {
            swingOffset = endOffset; // 목표 각도 유지
        }
        // 쿨타임 종료 시 공격 상태 해제 및 대기
        else {
            m_isAttacking = false;
            // 다음 대기 프레임도 이번 스윙의 종료 각도를 유지합니다.
            m_isSwung = !m_isSwung;
            swingOffset = endOffset;
        }
    }

    // 스윙 오프셋도 라디안으로 변환해 최종 회전을 적용합니다.
    constexpr float kDegreeToRadian = kPi / 180.f;
    m_sprite->setRotation(sf::radians(baseRotation + swingOffset * kDegreeToRadian));
    m_sprite->setScale({ 1.f, isFacingLeft ? -1.f : 1.f });
}

void Equip::render(sf::RenderWindow& window) {
    if (m_sprite) {
        window.draw(*m_sprite);
    }
}

std::optional<sf::FloatRect> Equip::getAttackHitbox() const {
    if (!m_sprite || !m_isAttacking) return std::nullopt;
    // 회전과 스케일이 모두 반영된 최종 Bounding Box 반환
    return m_sprite->getGlobalBounds();
}
