#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <unordered_set>
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

/// 프로그램 진입점입니다. 공용 리소스와 테스트 방을 준비한 뒤 입력·AI·충돌·렌더링 루프를 실행합니다.
int main() {
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
    if (const auto playerSpawn = room.getPlayerSpawnPosition(tileMap)) {
        player.move(playerSpawn->x, playerSpawn->y);
    }

    ObjectPoolingManager objectPool;
    // 게임 시작 시 자주 쓰는 몬스터·투사체를 비활성 상태로 선생성합니다.
    objectPool.prewarmMonsters(4, "SkelDog", { 100.f, 100.f, 10.f, 1.f }, "Monster");
    objectPool.prewarmProjectiles(32);
    MonsterManager monsterManager;
    CombatManager combatManager;
    Monster* meleeMonster = objectPool.acquireMonster(
        "SkelDog", { 100.f, 100.f, 10.f, 1.f }, "Monster");
    Monster* rangedMonster = objectPool.acquireMonster(
        "SkelDog", { 100.f, 100.f, 10.f, 1.f }, "Monster");
    meleeMonster->setEquipment(std::make_shared<Equip>(
        "MonsterClaw", EquipStat{ 10.f, 1.f, 50.f }));
    ProjectileConfig rangedConfig;
    rangedConfig.type = ProjectileType::Fireball;
    rangedConfig.target = ProjectileTarget::Player;
    rangedConfig.speed = 260.f;
    rangedConfig.damage = 8.f;
    rangedConfig.count = 1;
    rangedConfig.lifetime = 4.f;
    EquipStat rangedStat{ 8.f, 0.8f, 300.f, WeaponType::Ranged, rangedConfig };
    rangedMonster->setEquipment(std::make_shared<Equip>("MonsterFireball", rangedStat));
    if (const auto monsterSpawn = room.getMonsterSpawnPosition(tileMap)) {
        meleeMonster->move(monsterSpawn->x, monsterSpawn->y);
        rangedMonster->move(monsterSpawn->x + 180.f, monsterSpawn->y);
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
