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
#include "GameDataManager.h"

/// 프로그램 진입점입니다. 공용 리소스와 테스트 방을 준비한 뒤 입력·AI·충돌·렌더링 루프를 실행합니다.
int main() {
    // 소스는 UTF-8(/utf-8)로 컴파일되므로 콘솔도 UTF-8로 맞춥니다.
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    // true로 바꾸면 실제 플레이 대신 모든 레퍼런스 방을 축소해 한 화면에서 확인합니다.
    constexpr bool SHOW_ALL_ROOMS_DEBUG = false;
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
    if (!resMgr.loadAtlas("Equip", std::string(EQUIP_JSON), std::string(EQUIP_ATLAS))) {
        std::cerr << "장비 아틀라스 로드 실패\n";
    }
    if (!resMgr.loadAtlas("Projectile", std::string(PROJECTILE_JSON), std::string(PROJECTILE_ATLAS))) {
        std::cerr << "Projectile atlas load failed\n";
    }


    GameDataManager gameData;
    const std::filesystem::path dataDirectory =
        std::filesystem::path(__FILE__).parent_path() / "Resources" / "data";
    if (!gameData.loadWeapons((dataDirectory / "weapons.json").string()) ||
        !gameData.loadMonsters((dataDirectory / "monsters.json").string())) {
        std::cerr << "게임 데이터 JSON 로드 실패\n";
        return 1;
    }
    // 3. 고정 레퍼런스 방 데이터를 TileMap으로 변환
    TileMap tileMap;
    Room room(RoomType::Start);
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
    if (!room.buildTileMap(tileMap, "TileMap", roomTiles)) {
        std::cerr << "방 타일맵 생성 실패\n";
    }
    MapManager mapManager;
    if (SHOW_ALL_ROOMS_DEBUG && !mapManager.buildAllRoomsDebug(
        "TileMap", roomTiles, window.getSize())) {
        std::cerr << "방 디버그 프리뷰 생성 실패\n";
    }

    // 4. 플레이어 및 몬스터 생성
    Player player;
    player.init("Player");
    if (const auto playerWeapon = gameData.createEquip("ShortSword")) {
        player.setEquipment(playerWeapon);
    }
    if (const auto playerSpawn = room.getPlayerSpawnPosition(tileMap)) {
        player.move(playerSpawn->x, playerSpawn->y);
    }

    // 1.5배 확대된 뷰로 플레이어를 즉시 추적합니다.
    // 맵의 최하단에서는 뷰 하단이 맵 바닥과 정확히 맞춰집니다.
    Camera camera(window.getSize(),
        sf::FloatRect({ 0.f, 0.f }, tileMap.getPixelSize()), 3.f);
    camera.update(player.getCenterPosition());
    window.setView(camera.getView());

    const MonsterData* skelDogData = gameData.findMonster("SkelDog");
    const MonsterData* batData = gameData.findMonster("Bat");
    if (!skelDogData || !batData || !skelDogData->enabled || !batData->enabled) {
        std::cerr << "필수 몬스터 데이터가 없습니다.\n";
        return 1;
    }

    ObjectPoolingManager objectPool;
    objectPool.prewarmMonsters(4, skelDogData->id, skelDogData->status,
        skelDogData->atlasKey, skelDogData->behavior);
    objectPool.prewarmProjectiles(32);
    MonsterManager monsterManager;
    CombatManager combatManager;
    Monster* meleeMonster = objectPool.acquireMonster(skelDogData->id, skelDogData->status,
        skelDogData->atlasKey, skelDogData->behavior);
    Monster* rangedMonster = objectPool.acquireMonster(batData->id, batData->status,
        batData->atlasKey, batData->behavior);
    meleeMonster->setEquipment(gameData.createEquip(skelDogData->weaponId));
    rangedMonster->setEquipment(gameData.createEquip(batData->weaponId));
    if (const auto monsterSpawn = room.getMonsterSpawnPosition(tileMap)) {
        meleeMonster->move(monsterSpawn->x, monsterSpawn->y);
        rangedMonster->move(monsterSpawn->x + 180.f, monsterSpawn->y - 120.f);
    }

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

        // 1순위: 각 몬스터의 이동 후 벽 충돌을 해결합니다.
        monsterManager.update(dt, player, objectPool, tileMap);
        Collision::resolveMapCollision(player, tileMap);

        // 2순위: 플레이어의 공격을 먼저 처리합니다.
        std::unordered_set<EntityId> playerHitMonsters =
            combatManager.resolvePlayerAttack(player, objectPool);
        const auto projectileHits = combatManager.updateProjectiles(
            dt, player, objectPool, tileMap, ProjectileTarget::Monster);
        playerHitMonsters.insert(projectileHits.begin(), projectileHits.end());

        // 3순위: 플레이어가 이번 프레임에 실제로 맞힌 몬스터의 공격만 무효화합니다.
        combatManager.resolveMonsterAttacks(dt, player, objectPool, playerHitMonsters);
        combatManager.updateProjectiles(dt, player, objectPool, tileMap, ProjectileTarget::Player);

        // 충돌 보정까지 끝난 실제 플레이어 위치를 즉시 카메라에 반영합니다.
        camera.update(player.getCenterPosition());
        if (!SHOW_ALL_ROOMS_DEBUG) {
            window.setView(camera.getView());
        }

        // 렌더링
        window.clear(sf::Color::Black);

        if (SHOW_ALL_ROOMS_DEBUG) {
            mapManager.renderAllRoomsDebug(window);
        } else {
            window.draw(tileMap);
            player.render(window);
            objectPool.render(window);
        }

        window.display();
    }

    return 0;
}
