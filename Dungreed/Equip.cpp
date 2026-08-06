#include "Equip.h"
#include "Actor.h"
#include <cmath>
#include <iostream>

Equip::Equip(const std::string& name, EquipStat stat)
    : m_name(name), m_stat(stat) {}

void Equip::init(const std::string& atlasKey, const std::string& frameName) {
    auto& resMgr = ResourceManager::getInstance();
    const sf::Texture* tex = resMgr.getAtlasTexture(atlasKey);

    // Equip 폴더 내의 스프라이트임을 보장하기 위해 경로 자동 완성
    std::string actualFrameName = frameName;
    if (actualFrameName.find("Equip/") == std::string::npos) {
        actualFrameName = "Equip/" + actualFrameName;
    }
    if (actualFrameName.find(".png") == std::string::npos) {
        actualFrameName += ".png";
    }

    const sf::IntRect* rect = resMgr.getFrameRect(atlasKey, actualFrameName);

    if (tex && rect) {
        m_sprite.emplace(*tex);
        m_sprite->setTextureRect(*rect);
        // 무기를 쥐는 손잡이 부분(좌측 중앙)을 회전축(Origin)으로 설정
        m_sprite->setOrigin({ 0.f, rect->size.y / 2.f });
    } else {
        std::cerr << "[Equip] 무기 스프라이트를 찾을 수 없습니다: " << actualFrameName << std::endl;
    }
}

void Equip::attack() {
    // 이미 공격 중이 아닐 때만 공격 실행
    if (!m_isAttacking) {
        m_isAttacking = true;
        m_attackTimer = 0.f;
        // stat의 attackSpeed를 반영하여 공격 시간 설정 (기본 0.5초 기준)
        m_attackDuration = 0.5f / (m_stat.attackSpeed > 0.f ? m_stat.attackSpeed : 1.f);
    }
}

void Equip::update(float dt, const sf::Vector2f& ownerPos, float aimRadian) {
    if (!m_sprite) return;

    // 주인의 위치로 무기 위치 동기화
    m_sprite->setPosition(ownerPos);

    // 기본 조준 각도
    sf::Angle currentAngle = sf::radians(aimRadian);

    // 공격 중일 경우 회전 애니메이션 처리 (위아래로 흔들기)
    if (m_isAttacking) {
        m_attackTimer += dt;
        if (m_attackTimer >= m_attackDuration) {
            m_isAttacking = false;
            m_attackTimer = 0.f;
        } else {
            // 0 ~ 1 사이의 진행도
            float progress = m_attackTimer / m_attackDuration;

            // sin 함수를 사용하여 진행도(progress)에 따라 올라갔다 내려갔다 반복
            // progress * 3.14159f * 4.f를 하면 설정된 공격 시간 동안 2회 왕복 진동합니다.
            float swingOffset = std::sin(progress * 3.14159265f * 4.f) * 45.f; // 최대 45도 반경으로 진동

            // 마우스/조준 방향이 왼쪽일 때는 등 뒤로 꺾이지 않게 회전 오프셋 반전
            if (std::cos(aimRadian) < 0.f) {
                swingOffset = -swingOffset;
            }

            currentAngle += sf::degrees(swingOffset);
        }
    }

    // 마우스 혹은 타겟 방향으로 최종 회전 (SFML 3.1.0 Angle 구조체 적용)
    m_sprite->setRotation(currentAngle);

    // 조준 방향이 왼쪽(-x)일 때 무기 스프라이트가 상하 반전되어 뒤집히는 것 방지
    if (std::cos(aimRadian) < 0.f) {
        m_sprite->setScale({ 1.f, -1.f });
    } else {
        m_sprite->setScale({ 1.f, 1.f });
    }
}

void Equip::render(sf::RenderWindow& window) {
    if (m_sprite) {
        window.draw(*m_sprite);
    }
}

std::optional<sf::FloatRect> Equip::getAttackHitbox() const {
    if (!m_sprite) return std::nullopt;
    // 회전과 스케일이 모두 반영된 최종 Bounding Box 반환
    return m_sprite->getGlobalBounds();
}