#include "Actor.h"



Actor::Actor() {
    status.maxHp = MAXHP;
    status.tmpHp = status.maxHp;
    status.dex = DEX;
    status.power = POWER;
}

void Actor::init(const std::string& atlasKey) {
    auto& resourceManager = ResourceManager::getInstance();
    const sf::Texture* tex = resourceManager.getAtlasTexture(atlasKey);
    if (tex) {
        if (!sprite.has_value()) {
            sprite.emplace(*tex);
            setBottomCenterOrigin();
            col.updateHitbox(sprite->getGlobalBounds());
        }

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
    }
    else if (sprite) {
        return sprite->getGlobalBounds();
    }
    return std::nullopt;
}

void Actor::takeDamage(float damage) {
    // 공격자 정보가 없는 기존 호출도 기본 방향으로 넉백을 적용합니다.
    const sf::Vector2f center = getCenterPosition();
    takeDamage(damage, { center.x - 1.f, center.y });
}

void Actor::takeDamage(float damage, const sf::Vector2f& attackerPosition) {
    if (dead()) return;

    status.tmpHp -= damage;
    const float knockbackDirection =
        (getCenterPosition().x >= attackerPosition.x) ? 1.f : -1.f;
    m_knockbackVelocity = { knockbackDirection * KNOCKBACK_SPEED, 0.f };
    m_knockbackTimer = KNOCKBACK_DURATION;
    m_hitTimer = HIT_COLOR_DURATION;

    if (sprite) {
        sprite->setColor(sf::Color(255, 96, 96));
    }
}

//=================기본 액션 함수=====================

bool Actor::jump() {
    if (movement.isGrounded) {
        movement.velocity.y = movement.jumpForce; // 점프 시 위로 솟구침
        movement.isGrounded = false;
    }
    // 바닥에 닿지 않았으면 점프(공중) 상태이므로 true 반환
    return !movement.isGrounded;
}

//=================물리연산용 함수=====================
sf::Vector2f Actor::getCenterPosition()const {
    if (!sprite) return { 0.f, 0.f };
    sf::Vector2f pos = sprite->getPosition();
    sf::FloatRect bounds = sprite->getLocalBounds();
    return { pos.x, pos.y - (bounds.size.y / 2.f) };
}

void Actor::setBottomCenterOrigin() {
    if (sprite) { // sprite가 생성되어 있는지 확인
        sf::FloatRect bounds = sprite->getLocalBounds();
        // SFML 3.1.0은 Vector2f 구조체를 인자로 받습니다
        sprite->setOrigin({ bounds.size.x / 2.f, bounds.size.y });
    }
}

void Actor::setHorizontalInput(float dirX) {
    movement.velocity.x = dirX * movement.moveSpeed;
}
void Actor::updatePhysics(float dt) {
    // 1. 중력 적용
    if (!movement.isGrounded) {
        movement.velocity.y += movement.gravity * dt;
    }

    // 2. 이동 적용 (피격 넉백은 일반 이동과 합산)
    sf::Vector2f totalVelocity = movement.velocity;
    if (m_knockbackTimer > 0.f) {
        totalVelocity += m_knockbackVelocity;
    }
    move(totalVelocity.x * dt, totalVelocity.y * dt);
}

void Actor::playAnimation(const std::string& animationName) {
    if (animator.getCurrentAnimation() == animationName)
        return;

    animator.play(animationName);
}

void Actor::updateHitFeedback(float dt) {
    if (m_knockbackTimer > 0.f) {
        m_knockbackTimer -= dt;
        if (m_knockbackTimer <= 0.f) {
            m_knockbackTimer = 0.f;
            m_knockbackVelocity = { 0.f, 0.f };
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
//============기본 로직===============
void Actor::update(float dt) {
    // 물리 연산 수행
    updatePhysics(dt);

    // 애니메이션 갱신
    if (sprite) {
        animator.update(dt, *sprite);
        setBottomCenterOrigin();
        col.updateHitbox(sprite->getGlobalBounds());
    }

    updateHitFeedback(dt);
}

void Actor::render(sf::RenderWindow& window) {
    if (sprite) {
        window.draw(*sprite);
    }
    // 장비 렌더링 추가
    if (equipment) {
        equipment->render(window);
    }
}