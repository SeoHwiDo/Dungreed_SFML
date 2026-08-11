#include "Boss.h"

#include "TileMap.h"
#include "UIManager.h"

#include <algorithm>
#include <utility>

Boss::Boss(std::string displayName, Status initialStatus)
    : Actor(initialStatus), m_displayName(std::move(displayName)) {
    movement.gravity = 0.f;
    movement.velocity = { 0.f, 0.f };
}

Boss::~Boss() {
    UIManager::getInstance().detachBoss(this);
}

void Boss::init(const std::string& atlasKey) {
    Actor::init(atlasKey);
    movement.gravity = 0.f;
    movement.velocity = { 0.f, 0.f };
}

void Boss::placeAtMapCenter(const TileMap& tileMap) {
    const sf::Vector2f mapSize = tileMap.getPixelSize();
    setPosition({ mapSize.x * 0.5f, mapSize.y * 0.5f });
}

void Boss::beginSummon(float duration) {
    m_summonDuration = std::max(0.f, duration);
    m_summonTimer = m_summonDuration;
    UIManager::getInstance().attachBoss(this);
}

void Boss::setDisplayName(std::string displayName) {
    m_displayName = std::move(displayName);
}

float Boss::getSummonProgress() const {
    if (m_summonDuration <= 0.f) {
        return 1.f;
    }
    return 1.f - std::clamp(m_summonTimer / m_summonDuration, 0.f, 1.f);
}

void Boss::takeDamage(float damage, const sf::Vector2f& attackerPosition,
    float /*knockbackMultiplier*/) {
    // 보스는 Actor의 피해/피격 색상은 사용하지만 넉백은 받지 않습니다.
    Actor::takeDamage(damage, attackerPosition, 0.f);
    movement.velocity = { 0.f, 0.f };
}

void Boss::updateBossBase(float dt) {
    m_summonTimer = std::max(0.f, m_summonTimer - dt);
    movement.velocity = { 0.f, 0.f };
    updateHitFeedback(dt);
    updateAnimation(dt);
}

void Boss::renderBehindTiles(sf::RenderWindow& /*window*/) const {
}
