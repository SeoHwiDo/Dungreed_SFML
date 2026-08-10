#include <SFML/Graphics.hpp>
#include <Windows.h>
#include <iostream>
#include <vector>
#include <unordered_set>
#include <filesystem>
#include "ResourceManager.h"
#include "Player.h"
#include "Monster.h"
#include "Equip.h"
#include "ObjectPoolingManager.h"
#include "MonsterManager.h"
#include "CombatManager.h"
#include "TileMap.h"
#include "Collision.h"
#include "Room.h"
#include "MapManager.h"
#include "Camera.h"
#include "DebugManager.h"
#include "GameDataManager.h"

/// 프로그램 진입점입니다. 공용 리소스와 테스트 방을 준비한 뒤 입력·AI·충돌·렌더링 루프를 실행합니다.
int main() {
    // 소스는 UTF-8(/utf-8)로 컴파일되므로 콘솔도 UTF-8로 맞춥니다.
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    // true로 바꾸면 실제 플레이 대신 모든 레퍼런스 방을 축소해 한 화면에서 확인합니다.
    constexpr bool kShowAllRoomsDebug = false;
    // 실제 플레이 화면에서 피격·공격 판정 범위를 색상별로 표시합니다.
    constexpr bool kShowCombatBoundsDebug = true;
    // 1. 윈도우 생성 (SFML 3.1.0 기준 sf::VideoMode 및 윈도우 생성자 형식 준수)
    sf::RenderWindow window(sf::VideoMode({ 1280, 720 }), "Dungreed Test");
    window.setFramerateLimit(60);

    // 2. 리소스 로드
    auto& resMgr = ResourceManager::getInstance();
    if (!resMgr.loadAtlas("Player", std::string(kPlayerAtlasJsonPath), std::string(kPlayerAtlasPath))) {
        std::cerr << "플레이어 아틀라스 로드 실패\n";
    }
    if (!resMgr.loadAtlas("Monster", std::string(kMonsterAtlasJsonPath), std::string(kMonsterAtlasPath))) {
        std::cerr << "몬스터 아틀라스 로드 실패\n";
    }
    if (!resMgr.loadAtlas("TileMap", std::string(kTileMapAtlasJsonPath), std::string(kTileMapAtlasPath))) {
        std::cerr << "타일맵 아틀라스 로드 실패\n";
    }
    if (!resMgr.loadAtlas("Equip", std::string(kEquipAtlasJsonPath), std::string(kEquipAtlasPath))) {
        std::cerr << "장비 아틀라스 로드 실패\n";
    }
    if (!resMgr.loadAtlas("Projectile", std::string(kProjectileAtlasJsonPath), std::string(kProjectileAtlasPath))) {
        std::cerr << "프로젝타일 아틀라스 로드 실패\n";
    }


    GameDataManager gameData;
    const std::filesystem::path dataDirectory =
        std::filesystem::path(__FILE__).parent_path() / "Resources" / "data";
    if (!gameData.loadWeapons((dataDirectory / "weapons.json").string()) ||
        !gameData.loadMonsters((dataDirectory / "monsters.json").string()) ||
        !gameData.loadRoomData((dataDirectory / "room_data.json").string())) {
        std::cerr << "게임 데이터 JSON 로드 실패\n";
        return 1;
    }
    // 3. 고정 레퍼런스 방 데이터를 TileMap으로 변환
    MapManager mapManager;
    DebugManager debugManager;
    const FloorData* floorData = gameData.findFloor("floor_01");
    if (!floorData || !mapManager.createCurrentRoomFromData(
        *floorData, floorData->startRoomId)) {
        std::cerr << "시작 방 JSON 데이터 생성 실패\n";
        return 1;
    }
    Room* room = mapManager.getCurrentRoom();
    const RoomTileSet roomTiles{
        "Wall_Top.png",
        "Wall_Ground.png",
        "Wall_Left.png",
        "Wall_Right.png",
        "Wall_H0.png",
        "Wall_H2.png",
        "Wall_H6.png",
        "Wall_H8.png",
        "Wall_TopLCorner.png",
        "Wall_TopRCorner.png",
        "Wall_BotLCorner.png",
        "Wall_BotRCorner.png",
        "Back_Inner.png",
        "Back_Top.png",
        "Back_Ground.png",
        "Back_Left.png",
        "Back_Right.png",
        "Back_TopLcorner.png",
        "Back_TopRCorner.png",
        "BackBotLCorner.png",
        "Back_BotRCorner.png",
        "Back_DoorTopL.png",
        "Back_DoorTopR.png",
        "Back_DoorBotL.png",
        "Back_DoorBotR.png",
        "Flatform_L.png",
        "Flatform_In.png",
        "Flatform_R.png"
    };
    if (!mapManager.preloadFloorTileMaps("TileMap", roomTiles)) {
        std::cerr << "층 타일맵 사전 생성 실패\n";
        return 1;
    }
    const TileMap* initialTileMap = mapManager.getCurrentTileMap();
    if (!initialTileMap) {
        std::cerr << "시작 방 타일맵을 찾을 수 없습니다.\n";
        return 1;
    }
    if (kShowAllRoomsDebug && !debugManager.buildRoomPreviews(
        mapManager.getFloorRoomsInDataOrder(), "TileMap", roomTiles, window.getSize())) {
        std::cerr << "방 디버그 프리뷰 생성 실패\n";
    }

    // 4. 플레이어 및 몬스터 생성
    Player player;
    player.init("Player");
    if (const auto playerWeapon = gameData.createEquip("ShortSword")) {
        player.setEquipment(playerWeapon);
    }
    if (const auto playerSpawn = room->getPlayerSpawnPosition(*initialTileMap)) {
        player.move(playerSpawn->x, playerSpawn->y);
    }

    // 맵의 최하단에서는 뷰 하단이 맵 바닥과 정확히 맞춰집니다.
    Camera camera(window.getSize(),
        sf::FloatRect({ 0.f, 0.f }, initialTileMap->getPixelSize()), 1.f);
    camera.update(player.getCenterPosition());
    // 디버그 프리뷰는 월드 카메라가 아닌 창 좌표계를 사용해야 전체 배치가 잘리지 않습니다.
    window.setView(kShowAllRoomsDebug ? window.getDefaultView() : camera.getView());

    ObjectPoolingManager objectPool;
    objectPool.prewarmFromGameData(gameData);
    MonsterManager monsterManager;
    CombatManager combatManager;

    bool areMonstersActivated = false;
    sf::Clock clock;
    bool isGameplayActive = false;

    // 5. 메인 게임 루프
    while (window.isOpen()) {
        // SFML 3.1.0 이벤트 폴링 방식
        while (const std::optional<sf::Event> event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
        }

        const TileMap* activeTileMap = mapManager.getCurrentTileMap();
        if (!activeTileMap) {
            std::cerr << "활성 방 타일맵을 찾을 수 없습니다.\n";
            return 1;
        }
        const TileMap& tileMap = *activeTileMap;

        if (!isGameplayActive) {
            window.clear(sf::Color::Black);
            if (kShowAllRoomsDebug) {
                window.setView(window.getDefaultView());
                debugManager.renderRoomPreviews(window);
            } else {
                window.draw(tileMap);
                player.render(window);
                objectPool.render(window);
                if (kShowCombatBoundsDebug) {
                    debugManager.renderCombatBounds(window, player, objectPool);
                }
            }
            window.display();
            isGameplayActive = true;
            clock.restart();
            continue;
        }
        float dt = clock.restart().asSeconds();

        // 업데이트 처리
        player.update(dt, window, tileMap);

        // 1순위: 각 몬스터의 이동 후 벽 충돌을 해결합니다.
        if (!areMonstersActivated) {
            Collision::resolveMapCollision(player, tileMap, player.ignoresOneWayPlatforms());
            mapManager.requestCurrentRoomMonsters(monsterManager, objectPool, gameData, tileMap, player.getBodyCenterPosition());
            areMonstersActivated = true;
        } else {
            monsterManager.update(dt, player, objectPool, tileMap);
            Collision::resolveMapCollision(player, tileMap, player.ignoresOneWayPlatforms());
        }

        bool didChangeRoom = false;
        if (!kShowAllRoomsDebug) {
            if (Room* currentRoom = mapManager.getCurrentRoom()) {
                const auto enteredDoor = currentRoom->getEnteredDoor(
                    player.getGlobalBounds(),
                    player.getPreviousGlobalBounds(), tileMap);
                if (enteredDoor && mapManager.moveCurrentRoom(*enteredDoor)) {
                    monsterManager.clearActiveRoom(objectPool);
                    player.cancelDash();

                    const TileMap& nextTileMap = *mapManager.getCurrentTileMap();
                    if (Room* nextRoom = mapManager.getCurrentRoom()) {
                        if (const auto spawnPosition = nextRoom->getPlayerSpawnPosition(nextTileMap)) {
                            player.setPosition(*spawnPosition);
                        }
                    }

                    camera.setMapBounds(sf::FloatRect(
                        { 0.f, 0.f }, nextTileMap.getPixelSize()));
                    camera.update(player.getCenterPosition());
                    areMonstersActivated = false;
                    didChangeRoom = true;
                }
            }
        }

        if (!didChangeRoom) {
            // 2순위: 플레이어의 공격을 먼저 처리합니다.
            std::unordered_set<EntityId> playerHitMonsters =
                combatManager.resolvePlayerAttack(player, objectPool);
            const auto projectileHits = combatManager.updateProjectiles(
                dt, player, objectPool, tileMap, ProjectileTarget::Monster);
            playerHitMonsters.insert(projectileHits.begin(), projectileHits.end());

            // 3순위: 플레이어가 이번 프레임에 실제로 맞힌 몬스터의 공격만 무효화합니다.
            combatManager.resolveMonsterAttacks(dt, player, objectPool, playerHitMonsters);
            combatManager.updateProjectiles(dt, player, objectPool, tileMap, ProjectileTarget::Player);
        }

        // 충돌 보정까지 끝난 실제 플레이어 위치를 즉시 카메라에 반영합니다.
        camera.update(player.getCenterPosition());
        if (!kShowAllRoomsDebug) {
            window.setView(camera.getView());
        }

        // 렌더링
        window.clear(sf::Color::Black);

        if (kShowAllRoomsDebug) {
            window.setView(window.getDefaultView());
            debugManager.renderRoomPreviews(window);
        } else {
            window.draw(*mapManager.getCurrentTileMap());
            player.render(window);
            objectPool.render(window);
            if (kShowCombatBoundsDebug) {
                debugManager.renderCombatBounds(window, player, objectPool);
            }
        }

        window.display();
    }

    return 0;
}
