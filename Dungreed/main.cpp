#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include "ResourceManager.h"
#include "Player.h"
#include "Monster.h"
#include "TileMap.h"
#include "Collision.h"

int main() {
    // 1. 윈도우 생성 (SFML 3.1.0 기준 sf::VideoMode 및 윈도우 생성자 형식 준수)
    sf::RenderWindow window(sf::VideoMode({ 1280, 720 }), "Dungreed Test");
    window.setFramerateLimit(60);

    // 2. 리소스 로드
    auto& resMgr = ResourceManager::getInstance();
    if (!resMgr.loadAtlas("Player", std::string(PLAYER_JSON), std::string(PLAYER_ATLAS))) {
        std::cerr << "플레이어 아틀라스 로드 실패\n";
    }
    if (!resMgr.loadAtlas("Monster", std::string(MONSTER_JSON), std::string(MONSTER_ATLAS))) {
        std::cerr << "몬스터 아틀라스 로드 실패\n";
    }
    if (!resMgr.loadAtlas("TileMap", std::string(TILEMAP_JSON), std::string(TILEMAP_ATLAS))) {
        std::cerr << "타일맵 아틀라스 로드 실패\n";
    }

    // 3. 테스트용 타일맵 구성 (바닥 생성)
    const unsigned int mapWidth = 40;
    const unsigned int mapHeight = 20;
    sf::Vector2f tileSize(32.f, 32.f);
    std::vector<TileConfig> grid(mapWidth * mapHeight, { "", TileType::None });

    // tilemap_atlas.json에 정의된 정확한 프레임 이름 사용 ("Tile/1FloorTileMiddle.png")
    std::string floorTileName = "Tile/1FloorTileMiddle.png";

    // 하단 바닥 배치
    for (unsigned int x = 0; x < mapWidth; ++x) {
        grid[x + (mapHeight - 2) * mapWidth] = { floorTileName, TileType::Solid };
    }

    TileMap tileMap;
    tileMap.load("TileMap", grid, mapWidth, mapHeight, tileSize);

    // 4. 플레이어 및 몬스터 생성
    Player player;
    player.init("Player");
    player.move(200.f, 300.f);

    Monster monster("SkelDog", { 100.f, 100.f, 10.f, 1.f }, "Monster");
    monster.init("Monster");
    monster.move(500.f, 300.f);

    sf::Clock clock;

    // 5. 메인 게임 루프
    while (window.isOpen()) {
        // SFML 3.1.0 이벤트 폴링 방식
        while (const std::optional<sf::Event> event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
        }

        float dt = clock.restart().asSeconds();

        // 업데이트 처리
        player.update(dt, window);
        monster.setTargetPos(player.getPosition());
        monster.update(dt);

        // 충돌 처리
        Collision::resolveMapCollision(player, tileMap);
        Collision::resolveMapCollision(monster, tileMap);

        // 렌더링
        window.clear(sf::Color::Black);

        window.draw(tileMap);
        player.render(window);
        monster.render(window);

        window.display();
    }

    return 0;
}