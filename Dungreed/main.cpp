#include <SFML/Graphics.hpp>
#include <Windows.h>

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <memory>
#include <unordered_set>
#include <vector>

#include "Camera.h"
#include "Collision.h"
#include "CombatManager.h"
#include "EffectManager.h"
#include "GameDataManager.h"
#include "MapManager.h"
#include "MonsterManager.h"
#include "ObjectPoolingManager.h"
#include "Player.h"
#include "ResourceManager.h"
#include "RewardChestManager.h"
#include "SceneTransition.h"
#include "SkelBoss.h"
#include "UIManager.h"

namespace {
constexpr float kGameplayCameraZoom = 3.5f;

RoomTileSet createRoomTileSet() {
    return {
        "Wall_Outter.png", "Wall_Top.png", "Wall_Ground.png", "Wall_Left.png",
        "Wall_Right.png", "Wall_H0.png", "Wall_H2.png", "Wall_H6.png", "Wall_H8.png",
        "Wall_TopLCorner.png", "Wall_TopRCorner.png", "Wall_BotLCorner.png",
        "Wall_BotRCorner.png", "Back_Inner.png", "Back_Top.png", "Back_Ground.png",
        "Back_Left.png", "Back_Right.png", "Back_TopLcorner.png", "Back_TopRCorner.png",
        "BackBotLCorner.png", "Back_BotRCorner.png", "Back_DoorTopL.png",
        "Back_DoorTopR.png", "Back_DoorBotL.png", "Back_DoorBotR.png", "Platform.png"
    };
}

void drawCentered(sf::RenderWindow& window, sf::Text& text, const sf::Vector2f& position) {
    const sf::FloatRect bounds = text.getLocalBounds();
    text.setOrigin(bounds.getCenter());
    text.setPosition(position);
    window.draw(text);
}
}

/// 프로그램 진입점입니다. 타이틀, 조작 마을, 던전을 별도 장면으로 관리합니다.
int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    sf::RenderWindow window(sf::VideoMode({ 1280, 720 }), "Dungreed");
    window.setFramerateLimit(60);

    auto& resources = ResourceManager::getInstance();
    if (!resources.loadTitleResources()) {
        std::cerr << "기본 게임 폰트 로드 실패\n";
        return 1;
    }
    const sf::Font* font = resources.getDefaultFont();
    if (!font) {
        return 1;
    }

    SceneTransition transition;
    if (!transition.init(*font, window.getSize())) {
        return 1;
    }

    sf::Text titleText(*font, "DUNGREED", 74);
    titleText.setFillColor(sf::Color(244, 234, 255));
    titleText.setOutlineColor(sf::Color(40, 24, 66));
    titleText.setOutlineThickness(3.f);
    sf::Text titleHint(*font, "Press Enter to start", 26);
    titleHint.setFillColor(sf::Color(213, 194, 243));
    sf::Text villageTitle(*font, "TRAINING VILLAGE", 28);
    villageTitle.setFillColor(sf::Color::White);
    villageTitle.setOutlineColor(sf::Color(25, 18, 40));
    villageTitle.setOutlineThickness(2.f);
    sf::Text villageHelp(*font,
        "A / D or Arrow Keys: Move    Space: Jump    Shift: Dash    Mouse: Attack\n\n"
        "Practice freely. Press Enter when you are ready for the dungeon.", 19);
    villageHelp.setFillColor(sf::Color(235, 229, 255));
    villageHelp.setOutlineColor(sf::Color(25, 18, 40));
    villageHelp.setOutlineThickness(1.5f);

    const RoomTileSet roomTiles = createRoomTileSet();
    const std::filesystem::path dataDirectory =
        std::filesystem::path(__FILE__).parent_path() / "Resources" / "data";
    auto& gameData = GameDataManager::getInstance();
    auto& mapManager = MapManager::getInstance();
    auto& objectPool = ObjectPoolingManager::getInstance();
    auto& monsterManager = MonsterManager::getInstance();
    auto& combatManager = CombatManager::getInstance();
    auto& effectManager = EffectManager::getInstance();
    auto& rewardChestManager = RewardChestManager::getInstance();
    auto& uiManager = UIManager::getInstance();

    GameScene activeScene = GameScene::Title;
    TileMap* villageTileMap = nullptr;
    std::unique_ptr<Player> player;
    std::unique_ptr<Camera> camera;
    std::unique_ptr<SkelBoss> activeBoss;
    bool areMonstersActivated = false;

    const auto prepareVillage = [&]() -> bool {
        const FloorData* floor = gameData.findFloor("0Floor");
        if (!floor || !mapManager.createCurrentRoomFromData(*floor, floor->startRoomId) ||
            !mapManager.preloadFloorTileMaps("TileMap", roomTiles)) {
            return false;
        }
        Room* room = mapManager.getCurrentRoom();
        TileMap* tileMap = mapManager.getCurrentTileMap();
        if (!room || !tileMap) {
            return false;
        }

        auto newPlayer = std::make_unique<Player>();
        if (const auto weapon = gameData.createEquip("ShortSword")) {
            newPlayer->setEquipment(weapon);
        }
        if (const auto spawn = room->getPlayerSpawnPosition(*tileMap)) {
            newPlayer->setPosition(*spawn);
        }

        camera = std::make_unique<Camera>(window.getSize(),
            sf::FloatRect({ 0.f, 0.f }, tileMap->getPixelSize()), kGameplayCameraZoom);
        camera->update(newPlayer->getPosition());
        player = std::move(newPlayer);
        villageTileMap = tileMap;
        return true;
    };

    const auto prepareDungeon = [&]() -> bool {
        // 0Floor의 TileMap 포인터는 새 층을 생성하면 무효화되므로 먼저 비웁니다.
        villageTileMap = nullptr;
        const FloorData* floor = gameData.findFloor("floor_01");
        if (!floor || !mapManager.createCurrentRoomFromData(*floor, floor->startRoomId) ||
            !mapManager.preloadFloorTileMaps("TileMap", roomTiles)) {
            return false;
        }

        Room* startRoom = mapManager.getCurrentRoom();
        const TileMap* startMap = mapManager.getCurrentTileMap();
        if (!startRoom || !startMap) {
            return false;
        }

        auto newPlayer = std::make_unique<Player>();
        if (const auto weapon = gameData.createEquip("ShortSword")) {
            newPlayer->setEquipment(weapon);
        }
        if (const auto spawn = startRoom->getPlayerSpawnPosition(*startMap)) {
            newPlayer->setPosition(*spawn);
        }

        objectPool.prewarmFromGameData(gameData);
        if (!rewardChestManager.init() || !uiManager.init(window)) {
            return false;
        }

        camera = std::make_unique<Camera>(window.getSize(),
            sf::FloatRect({ 0.f, 0.f }, startMap->getPixelSize()), kGameplayCameraZoom);
        camera->update(newPlayer->getPosition());
        player = std::move(newPlayer);
        activeBoss.reset();
        areMonstersActivated = false;
        return true;
    };

    const auto beginVillageTransition = [&]() {
        transition.begin(GameScene::TrainingVillage, "TRAINING VILLAGE", {
            [&]() { return resources.loadTrainingVillageResources(); },
            [&]() { return gameData.loadWeapons((dataDirectory / "weapons.json").string()); },
            [&]() { return gameData.loadRoomData((dataDirectory / "room_data.json").string()); },
            prepareVillage
        }, [&]() { activeScene = GameScene::TrainingVillage; });
    };

    const auto beginDungeonTransition = [&]() {
        transition.begin(GameScene::Dungeon, "DUNGEON", {
            [&]() { return resources.loadDungeonResources(); },
            [&]() { return gameData.loadMonsters((dataDirectory / "monsters.json").string()); },
            prepareDungeon
        }, [&]() { activeScene = GameScene::Dungeon; });
    };

    sf::Clock clock;
    while (window.isOpen()) {
        bool enterPressed = false;
        while (const std::optional<sf::Event> event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            if (const auto* key = event->getIf<sf::Event::KeyPressed>();
                key && key->code == sf::Keyboard::Key::Enter) {
                enterPressed = true;
            }
        }

        const float dt = std::min(clock.restart().asSeconds(), 0.1f);
        if (enterPressed && !transition.isActive()) {
            if (activeScene == GameScene::Title) {
                beginVillageTransition();
            } else if (activeScene == GameScene::TrainingVillage) {
                beginDungeonTransition();
            }
        }
        transition.update(dt);

        if (!transition.isActive() && activeScene == GameScene::TrainingVillage &&
            player && villageTileMap && camera) {
            villageTileMap->update(dt);
            player->update(dt, window, *villageTileMap);
            Collision::resolveMapCollision(*player, *villageTileMap,
                player->ignoresOneWayPlatforms());
            camera->update(player->getPosition());
        }

        if (!transition.isActive() && activeScene == GameScene::Dungeon && player && camera) {
            TileMap* activeTileMap = mapManager.getCurrentTileMap();
            Room* currentRoom = mapManager.getCurrentRoom();
            if (!activeTileMap || !currentRoom) {
                std::cerr << "활성 던전 방을 찾을 수 없습니다.\n";
                return 1;
            }
            TileMap& tileMap = *activeTileMap;
            const bool isBossRoom = currentRoom->getInfo().type == RoomType::Boss;
            if (isBossRoom && !currentRoom->getInfo().isClear && !activeBoss) {
                activeBoss = std::make_unique<SkelBoss>();
                activeBoss->placeAtMapCenter(tileMap);
            } else if (!isBossRoom) {
                activeBoss.reset();
            }

            effectManager.update(dt, objectPool);
            window.setView(camera->getView());
            player->update(dt, window, tileMap);

            if (!areMonstersActivated) {
                Collision::resolveMapCollision(*player, tileMap, player->ignoresOneWayPlatforms());
                if (!isBossRoom) {
                    mapManager.requestCurrentRoomMonsters(monsterManager, objectPool, gameData,
                        tileMap, player->getBodyCenterPosition(), effectManager);
                }
                areMonstersActivated = true;
            } else if (!isBossRoom) {
                monsterManager.update(dt, *player, objectPool, tileMap, effectManager);
                Collision::resolveMapCollision(*player, tileMap, player->ignoresOneWayPlatforms());
            } else {
                Collision::resolveMapCollision(*player, tileMap, player->ignoresOneWayPlatforms());
            }

            if (activeBoss) {
                activeBoss->update(dt, *player, objectPool, effectManager, tileMap);
                if (activeBoss->dead()) {
                    currentRoom->setClear(true);
                }
            }

            currentRoom->setTraversalLocked(isBossRoom && activeBoss && !activeBoss->dead());
            tileMap.setDoorsLocked(currentRoom->isTraversalLocked());
            tileMap.update(dt);

            bool didChangeRoom = false;
            if (const auto enteredDoor = currentRoom->getEnteredDoor(player->getGlobalBounds(),
                player->getPreviousGlobalBounds(), tileMap);
                enteredDoor && mapManager.moveCurrentRoom(*enteredDoor)) {
                monsterManager.clearActiveRoom(objectPool);
                effectManager.clear(objectPool);
                player->cancelDash();

                const TileMap* nextMap = mapManager.getCurrentTileMap();
                if (Room* nextRoom = mapManager.getCurrentRoom(); nextMap && nextRoom) {
                    if (const auto spawn = nextRoom->getPlayerSpawnPosition(*nextMap)) {
                        player->setPosition(*spawn);
                    }
                    camera->setMapBounds(sf::FloatRect({ 0.f, 0.f }, nextMap->getPixelSize()));
                    camera->update(player->getPosition());
                }
                activeBoss.reset();
                areMonstersActivated = false;
                didChangeRoom = true;
            }

            if (!didChangeRoom) {
                std::unordered_set<EntityId> playerHitMonsters =
                    combatManager.resolvePlayerAttack(*player, objectPool, effectManager,
                        activeBoss.get());
                const auto projectileHits = combatManager.updateProjectiles(dt, *player,
                    objectPool, tileMap, ProjectileTarget::Monster, activeBoss.get());
                playerHitMonsters.insert(projectileHits.begin(), projectileHits.end());
                combatManager.resolveMonsterAttacks(dt, *player, objectPool, tileMap,
                    playerHitMonsters);
                combatManager.updateProjectiles(dt, *player, objectPool, tileMap,
                    ProjectileTarget::Player, activeBoss.get());
                rewardChestManager.update(dt, *currentRoom, tileMap, *player,
                    effectManager, objectPool);
            }

            const bool isBossCinematic = activeBoss && activeBoss->isSummoning();
            camera->setZoom(isBossCinematic ? 5.2f : kGameplayCameraZoom);
            camera->update(isBossCinematic ? activeBoss->getBodyCenterPosition() : player->getPosition());
        }

        window.clear(sf::Color(11, 8, 20));
        if (activeScene == GameScene::Title) {
            window.setView(window.getDefaultView());
            drawCentered(window, titleText, { window.getSize().x * 0.5f, 260.f });
            drawCentered(window, titleHint, { window.getSize().x * 0.5f, 410.f });
        } else if (activeScene == GameScene::TrainingVillage && player && villageTileMap && camera) {
            window.setView(camera->getView());
            window.draw(*villageTileMap);
            player->render(window);
            window.setView(window.getDefaultView());
            drawCentered(window, villageTitle, { window.getSize().x * 0.5f, 42.f });
            drawCentered(window, villageHelp, { window.getSize().x * 0.5f, 630.f });
        } else if (activeScene == GameScene::Dungeon && player && camera) {
            window.setView(camera->getView());
            objectPool.renderBehindTiles(window);
            if (const TileMap* tileMap = mapManager.getCurrentTileMap()) {
                window.draw(*tileMap);
            }
            if (activeBoss) {
                activeBoss->render(window);
            }
            player->render(window);
            objectPool.render(window);
            effectManager.render(window, objectPool);
            rewardChestManager.render(window);
            window.setView(window.getDefaultView());
            uiManager.update(*player, dt, window);
            uiManager.render(window);
        }

        window.setView(window.getDefaultView());
        transition.render(window);
        window.display();
    }

    return 0;
}
