#pragma once

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

struct InputData {
    float moveDirX = 0.f;
    bool isJumping = false;
    bool isDashing = false;
    bool isAttacking = false;
    sf::Vector2f aimDir;
    sf::Vector2f aimWorldPosition;
    float aimRadian = 0.f;
};

class Controller {
public:
    InputData getInput(const sf::RenderWindow& window, const sf::Vector2f& actorCenter);

private:
    bool m_prevMouseLeftPressed = false;
    bool m_prevDashPressed = false;
};