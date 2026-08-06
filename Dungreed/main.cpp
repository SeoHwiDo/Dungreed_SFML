#include <iostream>
#include <vector>
#include <SFML/Graphics.hpp>
#include <optional>

#include "ResourceManager.h"
#include "Animator.h"

// 애니메이션과 스프라이트를 하나로 묶어 관리할 구조체
struct AnimEntity {
    sf::Sprite sprite;
    Animator animator;

    // sf::Sprite는 기본 생성자가 삭제되어 있을 수 있으므로 텍스처를 받는 생성자 정의
    AnimEntity(const sf::Texture& texture) : sprite(texture) {}
};

int main() {
    // 1. 윈도우 생성 (1920x1080 해상도)
    sf::RenderWindow window(sf::VideoMode({ 1920, 1080 }), "Dungreed Animation Test - All Animations");
    window.setFramerateLimit(60);

    // 2. 리소스 로드
    auto& resMgr = ResourceManager::getInstance();

    if (!resMgr.loadAtlas("Player", std::string(PLAYER_JSON), std::string(PLAYER_ATLAS))) {
        std::cerr << "[에러] 플레이어 아틀라스 로드 실패!" << std::endl;
        return -1;
    }

    const sf::Texture* playerTexture = resMgr.getAtlasTexture("Player");
    if (!playerTexture) {
        std::cerr << "[에러] 플레이어 텍스처를 가져올 수 없습니다!" << std::endl;
        return -1;
    }

    // 3. 아틀라스에 등록된 모든 애니메이션 이름 가져오기
    std::vector<std::string> animNames = resMgr.getAnimationNames("Player");
    if (animNames.empty()) {
        std::cerr << "[에러] 아틀라스에서 애니메이션을 찾을 수 없습니다." << std::endl;
        return -1;
    }

    // 4. 화면 배치를 위한 세팅
    std::vector<AnimEntity> entities;

    int cols = 6; // 한 줄에 표시할 캐릭터 개수
    float startX = 200.f; // 화면 왼쪽 여백
    float startY = 200.f; // 화면 위쪽 여백
    float gapX = 250.f; // 캐릭터 간 가로 간격
    float gapY = 250.f; // 캐릭터 간 세로 간격

    for (size_t i = 0; i < animNames.size(); ++i) {
        // 생성 시점에 텍스처를 전달하여 E1790 컴파일 에러 해결
        AnimEntity entity(*playerTexture);

        // 스프라이트 크기 설정
        entity.sprite.setScale({ 4.f, 4.f });

        // 그리드(바둑판) 형태로 위치 계산
        int c = i % cols;
        int r = i / cols;
        entity.sprite.setPosition({ startX + c * gapX, startY + r * gapY });

        // 해당 이름의 애니메이션 프레임 등록
        const auto* frames = resMgr.getAnimationFrames("Player", animNames[i]);
        if (frames) {
            // 모든 애니메이션을 0.1초 간격으로 무한 루프 재생 설정
            AnimationClip clip(frames, 0.1f, true);
            entity.animator.addAnimation(animNames[i], clip);
            entity.animator.play(animNames[i]);
        }

        entities.push_back(std::move(entity));
        std::cout << "[정보] 애니메이션 배치 완료: " << animNames[i] << std::endl;
    }

    // 5. 게임 루프
    sf::Clock clock;

    while (window.isOpen()) {
        // 이벤트 처리
        while (const std::optional<sf::Event> event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
        }

        // 델타 타임(dt) 계산
        float dt = clock.restart().asSeconds();

        // 6. 업데이트 및 렌더링
        window.clear(sf::Color(40, 40, 40)); // 약간 어두운 회색 배경

        for (auto& entity : entities) {
            entity.animator.update(dt, entity.sprite);
            window.draw(entity.sprite);
        }

        window.display();
    }

    return 0;
}