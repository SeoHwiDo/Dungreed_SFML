#include "Actor.h"

#include <algorithm>
#include <iostream>

void Actor::init(const std::string &atlasKey) {
    auto &resourceManager = ResourceManager::getInstance();
    const sf::Texture *tex = resourceManager.getAtlasTexture(atlasKey);
    if (tex) {
        if (!sprite.has_value()) {
            sprite.emplace(*tex);
        } else {
            // 풀 객체가 다시 초기화될 때도 현재 리소스의 텍스처를 다시 연결합니다.
            sprite->setTexture(*tex);
        }
        setBottomCenterOrigin();
        col.updateHitbox(sprite->getGlobalBounds());
    }
}

void Actor::resetForReuse(Status newStatus) {
    status = newStatus;
    movement = MovementData{};
    m_hitTimer = 0.f;
    m_knockbackTimer = 0.f;
    m_knockbackVelocity = {0.f, 0.f};
    if (sprite) {
        sprite->setColor(sf::Color::White);
        col.updateHitbox(sprite->getGlobalBounds());
    }
}

void Actor::setEquipment(std::shared_ptr<Equip> eq) {
    equipment = eq;
    if (equipment) {
        equipment->setOwner(this);
    }
}

std::optional<sf::FloatRect> Actor::getAttackHitbox() const {
    if (equipment) {
        return equipment->getAttackHitbox();
    } else if (sprite) {
        return sprite->getGlobalBounds();
    }
    return std::nullopt;
}

void Actor::takeDamage(float damage) {
    // 공격자 정보가 없으면 액터의 왼쪽을 기준으로 넉백 방향을 정합니다.
    const sf::Vector2f center = getCenterPosition();
    takeDamage(damage, {center.x - 1.f, center.y});
}

void Actor::takeDamage(float damage, const sf::Vector2f &attackerPosition) { takeDamage(damage, attackerPosition, 1.f); }

void Actor::applyPermanentReward(float maxHpIncrease, float powerIncrease) {
    status.maxHp += std::max(0.f, maxHpIncrease);
    status.power += std::max(0.f, powerIncrease);
    status.tmpHp = status.maxHp;
}

void Actor::takeDamage(float damage, const sf::Vector2f &attackerPosition, float knockbackMultiplier) {
    if (dead())
        return;

    status.tmpHp -= damage;
    const float knockbackDirection = (getCenterPosition().x >= attackerPosition.x) ? 1.f : -1.f;
    const float clampedMultiplier = std::max(0.f, knockbackMultiplier);
    m_knockbackVelocity = {knockbackDirection * kKnockbackSpeed * clampedMultiplier, 100.f * clampedMultiplier};
    m_knockbackTimer = kKnockbackDuration;
    m_hitTimer = kHitColorDuration;

    std::cout << "[Hit] damage=" << damage << ", hp=" << status.tmpHp << ", attacker=(" << attackerPosition.x << ", " << attackerPosition.y << ")"
              << ", knockbackVelocity=(" << m_knockbackVelocity.x << ", " << m_knockbackVelocity.y << ")" << std::endl;

    if (sprite) {
        sprite->setColor(sf::Color(255, 96, 96));
    }
}

// 점프 처리

bool Actor::jump() {
    if (movement.isGrounded) {
        movement.velocity.y = movement.jumpForce; // 위쪽 초기 속도를 적용합니다.
        movement.isGrounded = false;
    }
    // 공중 상태이면 점프가 진행 중이므로 true를 반환합니다.
    return !movement.isGrounded;
}

// 물리 처리
sf::Vector2f Actor::getCenterPosition() const {
    if (!sprite)
        return {0.f, 0.f};
    sf::Vector2f pos = sprite->getPosition();
    sf::FloatRect bounds = sprite->getLocalBounds();
    return {pos.x, pos.y - (bounds.size.y / 2.f)};
}

void Actor::setBottomCenterOrigin() {
    if (sprite) { // 스프라이트가 초기화된 경우에만 원점을 설정합니다.
        sf::FloatRect bounds = sprite->getLocalBounds();
        // SFML 3.1은 Vector2f 형태로 스프라이트 원점을 설정합니다.
        sprite->setOrigin({bounds.size.x / 2.f, bounds.size.y});
    }
}

void Actor::setHorizontalInput(float dirX) { movement.velocity.x = dirX * movement.moveSpeed; }
void Actor::updatePhysics(float dt) {
    m_previousGlobalBounds = getGlobalBounds();
    // 공중 상태에서만 중력을 누적합니다.
    if (!movement.isGrounded) {
        movement.velocity.y += movement.gravity * dt;
    }

    // 일반 이동 속도와 피격 넉백 속도를 합산해 적용합니다.
    sf::Vector2f totalVelocity = movement.velocity;
    if (m_knockbackTimer > 0.f) {
        totalVelocity += m_knockbackVelocity;
    }
    move(totalVelocity.x * dt, totalVelocity.y * dt);
}

void Actor::playAnimation(const std::string &animationName) {
    if (animator.getCurrentAnimation() == animationName)
        return;

    animator.play(animationName);
}

void Actor::updateHitFeedback(float dt) {
    if (m_knockbackTimer > 0.f) {
        m_knockbackTimer -= dt;
        if (m_knockbackTimer <= 0.f) {
            m_knockbackTimer = 0.f;
            m_knockbackVelocity = {0.f, 0.f};
        }
    }

    if (m_hitTimer > 0.f) {
        m_hitTimer -= dt;
        if (m_hitTimer <= 0.f && sprite) {
            m_hitTimer = 0.f;
            sprite->setColor(sf::Color::White);
        }
    }
}
// void Actor::playSound(const std::string& soundName) {
//     if()
// }
void Actor::updateAnimation(float dt) {
    if (sprite) {
        animator.update(dt, *sprite);
        setBottomCenterOrigin();
        col.updateHitbox(sprite->getGlobalBounds());
    }
}

sf::Vector2f Actor::getBodyCenterPosition() const {
    const sf::FloatRect bounds = getGlobalBounds();
    return {bounds.position.x + bounds.size.x / 2.f, bounds.position.y + bounds.size.y / 2.f};
}

// 프레임 갱신
void Actor::update(float dt) {
    updatePhysics(dt);
    updateHitFeedback(dt);
    updateAnimation(dt);
}

void Actor::render(sf::RenderWindow &window) {
    if (sprite) {
        window.draw(*sprite);
    }
    // 장비는 본체 스프라이트 뒤에 이어서 렌더링합니다.
    if (equipment) {
        equipment->render(window);
    }
}
