#include "Controller.h"

InputData Controller::getInput(const sf::RenderWindow& window, const sf::Vector2f& actorCenter) {
    InputData data;

    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) {
        data.moveDirX -= 1.f;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right) ||
        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) {
        data.moveDirX += 1.f;
    }

    data.isJumping = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space);
    data.isDashing = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift)||
        sf::Mouse::isButtonPressed(sf::Mouse::Button::Right);
    data.isAttacking = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
    // [수정된 부분] 마우스를 뗄 때만 단발성으로 공격(true) 판정
    bool currentMouseLeftPressed = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
    if (!currentMouseLeftPressed && m_prevMouseLeftPressed) {
        data.isAttacking = true;
    }
    else {
        data.isAttacking = false;
    }
    m_prevMouseLeftPressed = currentMouseLeftPressed; // 현재 상태 저장
    // 마우스의 월드 좌표 추출
    sf::Vector2i mousePos = sf::Mouse::getPosition(window);
    sf::Vector2f mouseWorldPos = window.mapPixelToCoords(mousePos);

    // 중앙점으로부터 마우스까지의 차이 계산
    float dx = mouseWorldPos.x - actorCenter.x;
    float dy = mouseWorldPos.y - actorCenter.y;

    // 아크탄젠트를 이용해 라디안 값 추출 
    // 식: $\theta = \text{atan2}(\Delta y, \Delta x)$
    //360도 인식 및 0나누기 방지를 위한 atan2 사용
    data.aimRadian = std::atan2(dy, dx);

    // 라디안 값을 통해 단위 벡터 추출
    data.aimDir.x = std::cos(data.aimRadian);
    data.aimDir.y = std::sin(data.aimRadian);

    return data;
}
