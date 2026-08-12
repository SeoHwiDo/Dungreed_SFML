#include "SceneManager.h"

#include "DebugManager.h"
#include "GameDataManager.h"
#include "GameplayContext.h"
#include "LogManager.h"
#include "Player.h"
#include "ResourceManager.h"
#include "SkelBoss.h"
#include <iostream>

SceneManager::SceneManager(sf::RenderWindow &window, GameplayContext &gameplay) : m_window(window), m_gameplay(gameplay), m_titleScene(window), m_villageScene(window, gameplay), m_dungeonScene(window, gameplay), m_deathScene(window) { std::cout << "[SceneManager] TitleScene, VillageScene, DungeonScene, DeathScene created\n"; }

bool SceneManager::init() {
    if (!m_titleScene.enter()) {
        LogManager::getInstance().error("SceneManager", "타이틀 장면 초기화에 실패했습니다.");
        return false;
    }

    const sf::Font *font = ResourceManager::getInstance().getDefaultFont();
    if (!font) {
        LogManager::getInstance().error("SceneManager", "기본 폰트를 찾지 못해 장면 관리자를 초기화할 수 없습니다.");
        return false;
    }
    if (!m_transition.init(*font, m_window.getSize())) {
        LogManager::getInstance().error("SceneManager", "장면 전환 화면 초기화에 실패했습니다.");
        return false;
    }
    if (!m_deathScene.init(*font)) {
        LogManager::getInstance().error("SceneManager", "결과 장면 초기화에 실패했습니다.");
        return false;
    }
    m_easyModeText.emplace(*font, "easy", 32);
    m_easyModeText->setFillColor(sf::Color::Red);
    m_easyModeText->setStyle(sf::Text::Style::Bold);
    return true;
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

    if (const std::optional<DebugCommand> command =
            DebugManager::getInstance().handleEvent(event, GameDataManager::getInstance())) {
        handleDebugCommand(*command);
        return;
    }

    if (const auto *key = event.getIf<sf::Event::KeyPressed>()) {
        if (key->code == sf::Keyboard::Key::Enter) {
            if (m_activeScene == GameScene::Title) {
                changeToVillage();
            } else if (m_activeScene == GameScene::TrainingVillage) {
                changeToDungeon(1);
            }
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

        if (Player* player = m_gameplay.getPlayer();
            player && player->dead()) {

            // Player::update()가 Player_Die를 계속 갱신한다.
            if (player->isAnimationFinished("Player_Die")) {
                m_deathScene.enter(ResultType::Failure);
            }
        } else if (const SkelBoss* boss = m_dungeonScene.getActiveBoss();
            boss && boss->dead() && boss->isDeathSequenceFinished()) {
            m_deathScene.enter(ResultType::Success);
        }
    }
}

void SceneManager::render() {
    if (!m_deathScene.isActive() || m_deathScene.needsCapture()) {
        renderActiveScene();
    }

    if (m_deathScene.needsCapture()) {
        m_deathScene.captureCurrentScreen();
    }

    renderEasyModeIndicator();
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

void SceneManager::activateEasyMode() {
    Player *player = m_gameplay.getPlayer();
    if (!player) {
        LogManager::getInstance().error("SceneManager",
            "Easy mode could not be applied because the player is unavailable.");
        return;
    }

    player->activateEasyMode();
}

void SceneManager::renderEasyModeIndicator() {
#if defined(_DEBUG)
    const Player *player = m_gameplay.getPlayer();
    if (!player || !player->isEasyMode() || !m_easyModeText) {
        return;
    }

    m_window.setView(m_window.getDefaultView());
    const sf::FloatRect bounds = m_easyModeText->getLocalBounds();
    m_easyModeText->setPosition({20.f,
        static_cast<float>(m_window.getSize().y) - bounds.size.y - 20.f});
    m_window.draw(*m_easyModeText);
#endif
}

bool SceneManager::changeToVillage() {
    if (m_transition.isActive()) {
        return false;
    }
    return m_transition.begin(GameScene::TrainingVillage, u8"시작 마을", {[this]() {
                                  if (m_activeScene == GameScene::Dungeon) {
                                      m_dungeonScene.leave();
                                  }
                                  return m_villageScene.enter();
                              }},
                              [this]() { m_activeScene = GameScene::TrainingVillage; });
}

bool SceneManager::changeToDungeon(unsigned int floorNumber) {
    if (floorNumber == 0 || m_transition.isActive()) {
        return false;
    }
    return m_transition.begin(GameScene::Dungeon, u8"던전", {[this, floorNumber]() { return m_dungeonScene.enter(floorNumber); }}, [this]() { m_activeScene = GameScene::Dungeon; });
}

void SceneManager::handleDebugCommand(const DebugCommand &command) {
    switch (command.type) {
    case DebugCommandType::None:
        return;
    case DebugCommandType::ApplyEasyMode:
        activateEasyMode();
        return;
    case DebugCommandType::ToggleCombatBounds:
        if (m_activeScene == GameScene::Dungeon) {
            m_dungeonScene.toggleCombatBounds();
        }
        return;
    case DebugCommandType::SpawnRoom:
        if (m_activeScene != GameScene::Dungeon) {
            std::cout << "[Debug] Enter the dungeon before spawning a room.\n";
            return;
        }
        if (!m_dungeonScene.spawnDebugRoom(command.floorId, command.roomId)) {
            LogManager::getInstance().warning("SceneManager",
                "디버그 방 생성에 실패했습니다. 다른 방을 선택하세요.");
        }
        return;
    }
}
