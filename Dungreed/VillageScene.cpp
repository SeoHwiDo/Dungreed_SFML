#include "VillageScene.h"

#include "Camera.h"
#include "Collision.h"
#include "GameDataManager.h"
#include "GameplayContext.h"
#include "MapManager.h"
#include "Player.h"
#include "ResourceManager.h"
#include "Room.h"
#include "TileMap.h"

#include <filesystem>
#include <memory>

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
    text.setOrigin(text.getLocalBounds().getCenter());
    text.setPosition(position);
    window.draw(text);
}
}

VillageScene::VillageScene(sf::RenderWindow& window, GameplayContext& gameplay)
    : m_window(window), m_gameplay(gameplay) {}

VillageScene::~VillageScene() = default;

bool VillageScene::enter() {
    auto& resources = ResourceManager::getInstance();
    auto& gameData = GameDataManager::getInstance();
    auto& mapManager = MapManager::getInstance();
    const std::filesystem::path dataDirectory =
        std::filesystem::path(__FILE__).parent_path() / "Resources" / "data";

    if (!resources.loadTrainingVillageResources() ||
        !gameData.loadWeapons((dataDirectory / "weapons.json").string()) ||
        !gameData.loadRoomData((dataDirectory / "room_data.json").string())) {
        return false;
    }

    const FloorData* floor = gameData.findFloor("0Floor");
    const RoomTileSet roomTiles = createRoomTileSet();
    if (!floor || !mapManager.createCurrentRoomFromData(*floor, floor->startRoomId) ||
        !mapManager.preloadFloorTileMaps("TileMap", roomTiles)) {
        return false;
    }

    Room* room = mapManager.getCurrentRoom();
    TileMap* tileMap = mapManager.getCurrentTileMap();
    const sf::Font* font = resources.getDefaultFont();
    if (!room || !tileMap || !font) {
        return false;
    }

    if (!m_gameplay.placePlayerAtRoomSpawn(*room, *tileMap, kGameplayCameraZoom)) {
        return false;
    }
    m_tileMap = tileMap;

    m_titleText.emplace(*font, "TRAINING VILLAGE", 28);
    m_titleText->setFillColor(sf::Color::White);
    m_titleText->setOutlineColor(sf::Color(25, 18, 40));
    m_titleText->setOutlineThickness(2.f);
    m_helpText.emplace(*font,
        "A / D or Arrow Keys: Move    Space: Jump    Shift: Dash    Mouse: Attack\n\n"
        "Practice freely. Press Enter when you are ready for the dungeon.", 19);
    m_helpText->setFillColor(sf::Color(235, 229, 255));
    m_helpText->setOutlineColor(sf::Color(25, 18, 40));
    m_helpText->setOutlineThickness(1.5f);
    return true;
}

void VillageScene::update(float dt) {
    if (!isReady()) {
        return;
    }

    Player* player = m_gameplay.getPlayer();
    Camera* camera = m_gameplay.getCamera();
    m_tileMap->update(dt);
    player->update(dt, m_window, *m_tileMap);
    Collision::resolveMapCollision(*player, *m_tileMap,
        player->ignoresOneWayPlatforms());
    camera->update(player->getPosition());
}

void VillageScene::render() {
    if (!isReady() || !m_titleText || !m_helpText) {
        return;
    }

    Camera* camera = m_gameplay.getCamera();
    Player* player = m_gameplay.getPlayer();
    m_window.setView(camera->getView());
    m_window.draw(*m_tileMap);
    player->render(m_window);
    m_window.setView(m_window.getDefaultView());
    drawCentered(m_window, *m_titleText, { m_window.getSize().x * 0.5f, 42.f });
    drawCentered(m_window, *m_helpText, { m_window.getSize().x * 0.5f, 630.f });
}

bool VillageScene::isReady() const {
    return m_tileMap && m_gameplay.isReady();
}
