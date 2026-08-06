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


    // 확장자 .png가 없으면 자동으로 붙여주도록 처리
    std::string actualFrameName = frameName;
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
    // 1. 주인의 위치(ownerPos)를 기준으로 무기를 약간 몸 앞쪽으로 내밀어 쥐도록 오프셋(Offset) 추가
// 캐릭터가 보는 방향(마우스 방향)에 따라 무기의 시작 위치를 조절합니다.
    float offsetX = (std::cos(aimRadian) >= 0.f) ? 5.f : -5.f;
    sf::Vector2f adjustedPos = { ownerPos.x + offsetX, ownerPos.y };
    m_sprite->setPosition(adjustedPos);


    // 기본 조준 각도 (무기 이미지가 12시 방향을 향하므로 칼끝이 조준점을 향하도록 90도 보정)
    sf::Angle currentAngle = sf::radians(aimRadian) + sf::degrees(90.f);

    // 현재 대기 각도에서 반대쪽 끝까지 한 번만 스윙합니다.
    // startOffset은 대기 상태의 각도와 같아야 공격 시작 시 반대편으로
    // 되돌아가는(왕복처럼 보이는) 순간 이동이 발생하지 않습니다.
    float startOffset = m_isSwung ? 45.f : -45.f;
    float endOffset = m_isSwung ? -45.f : 45.f;
    float swingOffset = m_isSwung ? 45.f : -45.f; // 공격 대기 상태

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

    // 최종 스윙 각도 적용
    currentAngle += sf::degrees(swingOffset);
    m_sprite->setRotation(currentAngle);

    // 마우스 조준 방향에 따른 무기 좌우 스케일 반전 
    // (Y축을 반전시키면 손잡이 밖으로 무기가 벗어나므로 X축 반전 사용)
    if (std::cos(aimRadian) < 0.f) {
        m_sprite->setScale({ -1.f, 1.f });
    }
    else {
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