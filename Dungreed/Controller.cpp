#include "Controller.h"

#include <cmath>

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
    const bool dashPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) ||
        sf::Mouse::isButtonPressed(sf::Mouse::Button::Right);
    data.isDashing = dashPressed && !m_prevDashPressed;
    m_prevDashPressed = dashPressed;

    const bool mouseLeftPressed = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
    data.isAttacking = !mouseLeftPressed && m_prevMouseLeftPressed;
    m_prevMouseLeftPressed = mouseLeftPressed;

    const sf::Vector2i mousePosition = sf::Mouse::getPosition(window);
    data.aimWorldPosition = window.mapPixelToCoords(mousePosition);
    const float dx = data.aimWorldPosition.x - actorCenter.x;
    const float dy = data.aimWorldPosition.y - actorCenter.y;
    data.aimRadian = std::atan2(dy, dx);
    data.aimDir = { std::cos(data.aimRadian), std::sin(data.aimRadian) };
    return data;
}