#include "SceneManager.h"

#include "DebugManager.h"
#include "GameDataManager.h"
#include "GameplayContext.h"
#include "Player.h"
#include "ResourceManager.h"

#include <iostream>

SceneManager::SceneManager(sf::RenderWindow &window, GameplayContext &gameplay) : m_window(window), m_gameplay(gameplay), m_titleScene(window), m_villageScene(window, gameplay), m_dungeonScene(window, gameplay), m_deathScene(window) { std::cout << "[SceneManager] TitleScene, VillageScene, DungeonScene, DeathScene created\n"; }

bool SceneManager::init() {
    if (!m_titleScene.enter()) {
        return false;
    }

    const sf::Font *font = ResourceManager::getInstance().getDefaultFont();
    return font && m_transition.init(*font, m_window.getSize()) && m_deathScene.init(*font);
}

void SceneManager::handleEvent(const sf::Event &event) {
    if (m_transition.isActive()) {
        return;
    }

    if (m_deathScene.isActive()) {
        if (m_deathScene.handleEvent(event)) {
            m_deathScene.leave();
            changeToVillage();
        }
        return;
    }

    if (const auto *key = event.getIf<sf::Event::KeyPressed>()) {
        if (key->code == sf::Keyboard::Key::Enter) {
            if (m_activeScene == GameScene::Title) {
                changeToVillage();
            } else if (m_activeScene == GameScene::TrainingVillage) {
                changeToDungeon(1);
            }
        } else if (key->code == sf::Keyboard::Key::F6) {
            handleDebugCommand();
        }
    }
}

void SceneManager::update(float dt) {
    m_transition.update(dt);
    if (m_transition.isActive()) {
        return;
    }
    if (m_deathScene.isActive()) {
        return;
    }

    switch (m_activeScene) {
    case GameScene::Title:
        m_titleScene.update(dt);
        break;
    case GameScene::TrainingVillage:
        m_villageScene.update(dt);
        break;
    case GameScene::Dungeon:
        m_dungeonScene.update(dt);
        if (Player *player = m_gameplay.getPlayer(); player && player->dead()) {
            m_deathScene.enter(ResultType::Failure);
        } else if (m_dungeonScene.consumeBossDefeat()) {
            m_deathScene.enter(ResultType::Success);
        }
        break;
    }
}

void SceneManager::render() {
    if (!m_deathScene.isActive() || m_deathScene.needsCapture()) {
        renderActiveScene();
    }

    if (m_deathScene.needsCapture()) {
        m_deathScene.captureCurrentScreen();
    }
    m_deathScene.render();

    m_window.setView(m_window.getDefaultView());
    m_transition.render(m_window);
}

void SceneManager::renderActiveScene() {
    switch (m_activeScene) {
    case GameScene::Title:
        m_titleScene.render();
        break;
    case GameScene::TrainingVillage:
        m_villageScene.render();
        break;
    case GameScene::Dungeon:
        m_dungeonScene.render();
        break;
    }
}

bool SceneManager::changeToVillage() {
    if (m_transition.isActive()) {
        return false;
    }
    return m_transition.begin(GameScene::TrainingVillage, u8"시작 마을", {[this]() { return m_villageScene.enter(); }}, [this]() { m_activeScene = GameScene::TrainingVillage; });
}

bool SceneManager::changeToDungeon(unsigned int floorNumber) {
    if (floorNumber == 0 || m_transition.isActive()) {
        return false;
    }
    return m_transition.begin(GameScene::Dungeon, u8"던전", {[this, floorNumber]() { return m_dungeonScene.enter(floorNumber); }}, [this]() { m_activeScene = GameScene::Dungeon; });
}

void SceneManager::handleDebugCommand() {
    auto &debugManager = DebugManager::getInstance();
    const auto &gameData = GameDataManager::getInstance();
    while (true) {
        const DebugCommand command = debugManager.readConsoleCommand(gameData);
        if (command.type == DebugCommandType::None) {
            return;
        }
        if (command.type == DebugCommandType::ToggleCombatBounds) {
            if (m_activeScene == GameScene::Dungeon) {
                m_dungeonScene.toggleCombatBounds();
            }
            return;
        }
        if (m_activeScene != GameScene::Dungeon) {
            std::cout << "[Debug] Enter the dungeon before spawning a room.\n";
            return;
        }
        if (m_dungeonScene.spawnDebugRoom(command.floorId, command.roomId)) {
            return;
        }
        std::cerr << "[Debug] Room spawn failed. Select another room.\n";
    }
}
