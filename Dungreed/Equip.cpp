#include "Equip.h"
#include "Actor.h"
#include <cmath>
#include <iostream>

Equip::Equip(const std::string& name, EquipStat stat)
    : m_name(name), m_stat(stat) {}

void Equip::init(const std::string& atlasKey, const std::string& frameName) {
    auto& resMgr = ResourceManager::getInstance();
    const sf::Texture* tex = resMgr.getAtlasTexture(atlasKey);
    // equip_atlas.json 프레임 형식("ShortSword.png")에 맞게 수정


    const sf::IntRect* rect = resMgr.getFrameRect(atlasKey, frameName);

    if (tex && rect) {
        m_sprite.emplace(*tex);
        m_sprite->setTextureRect(*rect);
        // 무기를 쥐는 손잡이 부분(좌측 중앙)을 회전축(Origin)으로 설정
        m_sprite->setOrigin({ 0.f, rect->size.y / 2.f });
    } else {
        std::cerr << "[Equip] 무기 스프라이트를 찾을 수 없습니다: " << frameName << std::endl;
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

    // 스윙 각도 범위 설정 (-45도에서 +45도 구간)
    float startOffset = m_isSwung ? -45.f : 45.f;
    float endOffset = m_isSwung ? 45.f : -45.f;
    float swingOffset = m_isSwung ? 45.f : -45.f; // 공격 중이 아닐 때는 완료된 상태로 고정
    // 공격 중일 경우 회전 애니메이션 처리 (위아래로 흔들기)
    if (m_isAttacking) {
        m_attackTimer += dt;
        if (m_attackTimer >= m_attackDuration) {
            m_isAttacking = false;
            m_attackTimer = 0.f;
            swingOffset = endOffset; // 스윙 완료 시 목표 각도에 고정
        }
        else {
            // 0 ~ 1 사이의 진행도에 따라 선형 보간(Lerp) 적용
            float progress = m_attackTimer / m_attackDuration;
            swingOffset = startOffset + (endOffset - startOffset) * progress;
        }
    }

    // 마우스/조준 방향이 왼쪽일 때는 등 뒤로 꺾이지 않게 회전 오프셋 반전 보정
    if (std::cos(aimRadian) < 0.f) {
        swingOffset = -swingOffset;
    }

    // 최종 스윙 오프셋 적용 (SFML 3.1.0 기준)
    currentAngle += sf::degrees(swingOffset);
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