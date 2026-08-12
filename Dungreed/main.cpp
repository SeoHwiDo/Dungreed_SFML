#include <SFML/Graphics.hpp>
#include <Windows.h>

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>

#include "Camera.h"
#include "GameDataManager.h"
#include "GameplayContext.h"
#include "Player.h"
#include "ResourceManager.h"
#include "SceneManager.h"

/// 프로그램 진입점은 창과 상위 객체만 조립하고, 실제 흐름은 SceneManager에 위임합니다.
int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    sf::RenderWindow window(sf::VideoMode({1280, 720}), "Dungreed");
    window.setFramerateLimit(60);

    auto &resources = ResourceManager::getInstance();
    auto &gameData = GameDataManager::getInstance();
    const std::filesystem::path dataDirectory = std::filesystem::path(__FILE__).parent_path() / "Resources" / "data";
    if (!resources.loadSharedGameplayResources() || !gameData.loadWeapons((dataDirectory / "weapons.json").string())) {
        return 1;
    }

    auto player = std::make_unique<Player>();
    std::cout << "[main] Player created (id=" << player->getId() << ")\n";
    if (const auto weapon = gameData.createEquip("ShortSword")) {
        player->setEquipment(weapon);
        std::cout << "[main] Initial equipment created: ShortSword\n";
    }

    auto camera = std::make_unique<Camera>(window.getSize(), sf::FloatRect({0.f, 0.f}, {1.f, 1.f}), 1.0f);
    std::cout << "[main] Camera created\n";
    GameplayContext gameplay(*player, *camera);
    SceneManager scenes(window, gameplay);
    if (!scenes.init()) {
        return 1;
    }

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
