#include <SFML/Graphics.hpp>
#include <Windows.h>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <optional>

#include "Camera.h"
#include "AudioManager.h"
#include "GameDataManager.h"
#include "GameplayContext.h"
#include "LogManager.h"
#include "Player.h"
#include "ResourceManager.h"
#include "SceneManager.h"
#include "ObjectPoolingManager.h"

/// 프로그램 진입점은 창과 상위 객체만 조립하고, 실제 흐름은 SceneManager에 위임합니다.
int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    sf::RenderWindow window(sf::VideoMode({1280, 720}), "Dungreed");
    window.setFramerateLimit(60);

    auto &resources = ResourceManager::getInstance();
    auto &gameData = GameDataManager::getInstance();
    if (!resources.loadSharedGameplayResources() || !resources.loadSharedAudioResources() ||
        !AudioManager::getInstance().initialize() || !gameData.loadDungeonData()) {
        LogManager::getInstance().error("main", "Critical startup data could not be loaded. Terminating game.");
        return EXIT_FAILURE;
    }

    const PlayerData *playerData = gameData.getPlayerData();
    if (!playerData) {
        LogManager::getInstance().error("main", "Critical player data is missing. Terminating game.");
        return EXIT_FAILURE;
    }

    auto player = std::make_unique<Player>(playerData->defaultStatus, playerData->atlasKey);
    player->configureStatPresets(playerData->defaultStatus, playerData->easyStatus);
    std::cout << "[main] Player created (id=" << player->getId() << ")\n";
    if (const auto weapon = gameData.createEquip(playerData->defaultWeaponId)) {
        player->setEquipment(weapon);
        std::cout << "[main] Initial equipment created: " << playerData->defaultWeaponId << "\n";
    } else {
        LogManager::getInstance().error("main", "Critical player default weapon could not be created. Terminating game.");
        return EXIT_FAILURE;
    }

    auto camera = std::make_unique<Camera>(window.getSize(), sf::FloatRect({0.f, 0.f}, {1.f, 1.f}), 1.0f);
    std::cout << "[main] Camera created\n";
    GameplayContext gameplay(*player, *camera);
    SceneManager scenes(window, gameplay);
    if (!scenes.init()) {
        return 1;
    }
    //auto& objectPool = ObjectPoolingManager::getInstance();
    //objectPool.prewarmFromGameData(gameData);
    //constexpr std::size_t kBossProjectilePoolCapacity = 384;
    //objectPool.prewarmProjectiles(kBossProjectilePoolCapacity);
    sf::Clock clock;
    while (window.isOpen()) {
        while (const std::optional<sf::Event> event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            } else {
                scenes.handleEvent(*event);
            }
        }

        const float dt = std::min(clock.restart().asSeconds(), 0.1f);
        scenes.update(dt);
        window.clear(sf::Color(11, 8, 20));
        scenes.render();
        window.display();
    }

    return 0;
}
