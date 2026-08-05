#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <iostream>

#include "ResourceManager.h"
#include "Animator.h"

int main() {
    // 1. SFML 3.1.0 기준 윈도우 생성 (VideoMode에 {} 중괄호 사용)
    sf::RenderWindow window(sf::VideoMode({ 800, 600 }), L"Dungreed SFML - 애니메이션 테스트");
    window.setFramerateLimit(60); // 60프레임 제한

    // 2. 리소스 매니저를 통한 아틀라스 로드
    auto& rm = ResourceManager::getInstance();

    // ResourceManager.h에 선언해두신 constexpr string_view 경로를 std::string으로 변환하여 사용
    std::string atlasKey = "player";
    if (!rm.loadAtlas(atlasKey, std::string(PLAYER_JSON), std::string(PLAYER_ATLAS))) {
        std::cerr << "플레이어 아틀라스 로드 실패! 경로를 확인해주세요.\n";
        return -1;
    }

    // 3. 테스트용 스프라이트 세팅 (SFML 3.1.0 대응 수정)
    // 텍스처를 먼저 가져옵니다.
    const sf::Texture* playerTex = rm.getAtlasTexture(atlasKey);
    if (!playerTex) {
        std::cerr << "플레이어 텍스처를 가져오지 못했습니다!\n";
        return -1;
    }

    // SFML 3.1.0: 스프라이트 생성 시 반드시 텍스처(참조)를 넘겨주어야 합니다!
    sf::Sprite playerSprite(*playerTex);

    // 화면 중앙 배치 및 크기 확대 (도트 그래픽이 잘 보이도록)
    playerSprite.setPosition({ 400.f, 300.f });
    playerSprite.setScale({ 3.f, 3.f }); // 3배 확대

    // 4. 애니메이터 설정
    Animator playerAnimator;

    // 실제 json에 파싱된 애니메이션 이름(run, attack 등)으로 변경하세요.
    std::string testAnimName = "Player_Walk";

    const auto* animFrames = rm.getAnimationFrames(atlasKey, testAnimName);
    if (animFrames) {
        // 프레임 배열, 프레임당 시간(0.1초), 루프 여부(true)
        playerAnimator.addAnimation("TestAnim", AnimationClip(animFrames, 0.05f, true));
        playerAnimator.Play("TestAnim"); // 재생 시작!
    } else {
        std::cerr << "애니메이션 프레임을 찾을 수 없습니다: " << testAnimName << "\n";
    }

    // 시간 측정용 시계 (Delta Time 계산)
    sf::Clock clock;

    // 5. 메인 게임 루프
    while (window.isOpen()) {
        // SFML 3.1.0 기준 이벤트 처리
        while (const std::optional<sf::Event> event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
        }

        // dt(Delta Time) 계산: 이전 루프부터 지금까지 걸린 시간(초)
        float dt = clock.restart().asSeconds();

        // 애니메이터 갱신 (스프라이트의 텍스처 영역을 알아서 바꿔줍니다)
        playerAnimator.Update(dt, playerSprite);

        // 렌더링
        window.clear(sf::Color(50, 50, 50)); // 짙은 회색 배경
        window.draw(playerSprite);
        window.display();
    }

    return 0;
}