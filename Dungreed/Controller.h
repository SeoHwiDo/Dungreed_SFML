#pragma once
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
struct InputData {
    float moveDirX = 0.f;         // -1.f(좌), 1.f(우), 0.f(정지)
    bool isJumping = false;
    bool isDashing = false;
    bool isAttacking = false;
    sf::Vector2f aimDir;   // 마우스를 향하는 방향 단위 벡터
    float aimRadian = 0.f; // 필요시 무기 회전 등에 사용할 라디안 값
};

class Controller {
public:
    Controller() = default;
    ~Controller() = default;

    // 매 프레임마다 호출하여 현재 입력 상태를 가져옵니다.
    // 마우스 좌표를 월드 좌표로 변환하기 위해 윈도우 참조가 필요합니다.
    InputData getInput(const sf::RenderWindow& window, const sf::Vector2f& actorCenter);
private:
    bool m_prevMouseLeftPressed = false;
};
