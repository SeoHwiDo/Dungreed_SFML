#include "SceneManager.h"

#include "DebugManager.h"
#include "GameplayContext.h"
#include "ResourceManager.h"

#include <iostream>

SceneManager::SceneManager(sf::RenderWindow& window, GameplayContext& gameplay)
    : m_window(window),
      m_titleScene(window),
      m_villageScene(window, gameplay),
      m_dungeonScene(window, gameplay) {
    std::cout << "[SceneManager] TitleScene, VillageScene, DungeonScene created\n";
}

bool SceneManager::init() {
    if (!m_titleScene.enter()) {
        return false;
    }

    const sf::Font* font = ResourceManager::getInstance().getDefaultFont();
    return font && m_transition.init(*font, m_window.getSize());
}

void SceneManager::handleEvent(const sf::Event& event) {
    if (m_transition.isActive()) {
        return;
    }

    if (m_activeScene == GameScene::Title && m_titleScene.handleEvent(event)) {
        changeToVillage();
        return;
    }

    if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
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

    switch (m_activeScene) {
    case GameScene::Title:
        m_titleScene.update(dt);
        break;
    case GameScene::TrainingVillage:
        m_villageScene.update(dt);
        break;
    case GameScene::Dungeon:
        m_dungeonScene.update(dt);
        break;
    }
}

void SceneManager::render() {
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

    m_window.setView(m_window.getDefaultView());
    m_transition.render(m_window);
}

bool SceneManager::changeToVillage() {
    if (m_transition.isActive()) {
        return false;
    }
    return m_transition.begin(GameScene::TrainingVillage, "TRAINING VILLAGE",
        { [this]() { return m_villageScene.enter(); } },
        [this]() { m_activeScene = GameScene::TrainingVillage; });
}

bool SceneManager::changeToDungeon(unsigned int floorNumber) {
    if (floorNumber == 0 || m_transition.isActive()) {
        return false;
    }
    return m_transition.begin(GameScene::Dungeon, "DUNGEON",
        { [this, floorNumber]() { return m_dungeonScene.enter(floorNumber); } },
        [this]() { m_activeScene = GameScene::Dungeon; });
}

void SceneManager::handleDebugCommand() {
    const DebugCommand command = DebugManager::getInstance().readConsoleCommand();
    if (command.type == DebugCommandType::SpawnRoom) {
        if (m_activeScene != GameScene::Dungeon) {
            std::cout << "[Debug] Enter the dungeon before spawning a room.\n";
            return;
        }
        if (!m_dungeonScene.spawnDebugRoom(command.floorId, command.roomId)) {
            std::cerr << "[Debug] Room spawn failed: " << command.floorId
                << " / " << command.roomId << '\n';
        }
    } else if (command.type == DebugCommandType::ToggleCombatBounds &&
        m_activeScene == GameScene::Dungeon) {
        m_dungeonScene.toggleCombatBounds();
    }
}
