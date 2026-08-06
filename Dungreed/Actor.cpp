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
        }

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

    // 2. 이동 적용
    move(movement.velocity.x * dt, movement.velocity.y * dt);

    // 3. [임시 처리] 바닥 충돌 보정 (타일맵 충돌 연동 전 테스트용)
    if (sprite && sprite->getPosition().y >= 800.f) {
        sprite->setPosition({ sprite->getPosition().x, 800.f });
        movement.velocity.y = 0.f;
        movement.isGrounded = true;
    }
}

//============기본 로직===============
void Actor::update(float dt) {
    // 물리 연산 수행
    updatePhysics(dt);

    // 애니메이션 갱신
    if (sprite) {
        animator.update(dt, *sprite);
    }
}

void Actor::render(sf::RenderWindow& window) {
    if (sprite) {
        window.draw(*sprite);
    }
}