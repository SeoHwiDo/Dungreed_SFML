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
#include <string_view>

namespace {
constexpr float kGameplayCameraZoom = 3.5f;

sf::String toSfUtf8String(std::string_view utf8Text) { return sf::String::fromUtf8(utf8Text.begin(), utf8Text.end()); }

RoomTileSet createRoomTileSet() { return {"Wall_Outter.png", "Wall_Top.png", "Wall_Ground.png", "Wall_Left.png", "Wall_Right.png", "Wall_H0.png", "Wall_H2.png", "Wall_H6.png", "Wall_H8.png", "Wall_TopLCorner.png", "Wall_TopRCorner.png", "Wall_BotLCorner.png", "Wall_BotRCorner.png", "Back_Inner.png", "Back_Top.png", "Back_Ground.png", "Back_Left.png", "Back_Right.png", "Back_TopLcorner.png", "Back_TopRCorner.png", "BackBotLCorner.png", "Back_BotRCorner.png", "Back_DoorTopL.png", "Back_DoorTopR.png", "Back_DoorBotL.png", "Back_DoorBotR.png", "Platform.png"}; }

void drawCentered(sf::RenderWindow &window, sf::Text &text, const sf::Vector2f &position) {
    text.setOrigin(text.getLocalBounds().getCenter());
    text.setPosition(position);
    window.draw(text);
}
} // namespace

VillageScene::VillageScene(sf::RenderWindow &window, GameplayContext &gameplay) : m_window(window), m_gameplay(gameplay) {}

VillageScene::~VillageScene() = default;

bool VillageScene::enter() {
    auto &resources = ResourceManager::getInstance();
    auto &gameData = GameDataManager::getInstance();
    auto &mapManager = MapManager::getInstance();
    const std::filesystem::path dataDirectory = std::filesystem::path(__FILE__).parent_path() / "Resources" / "data";

    if (!resources.loadTrainingVillageResources() || !gameData.loadWeapons((dataDirectory / "weapons.json").string()) || !gameData.loadRoomData((dataDirectory / "room_data.json").string())) {
        return false;
    }

    const FloorData *floor = gameData.findFloor("floor_00");
    const RoomTileSet roomTiles = createRoomTileSet();
    if (!floor || !mapManager.createCurrentRoomFromData(*floor, floor->startRoomId) || !mapManager.preloadFloorTileMaps("TileMap", roomTiles)) {
        return false;
    }

    Room *room = mapManager.getCurrentRoom();
    TileMap *tileMap = mapManager.getCurrentTileMap();
    const sf::Font *font = resources.getDefaultFont();
    if (!room || !tileMap || !font) {
        return false;
    }

    Player *player = m_gameplay.getPlayer();
    if (!player) {
        return false;
    }
    player->restoreForVillage();
    if (!m_gameplay.placePlayerAtRoomSpawn(*room, *tileMap, kGameplayCameraZoom)) {
        return false;
    }
    m_tileMap = tileMap;

    m_titleText.emplace(*font, toSfUtf8String(u8"시작 마을"), 28);
    m_titleText->setFillColor(sf::Color::White);
    m_titleText->setOutlineColor(sf::Color(25, 18, 40));
    m_titleText->setOutlineThickness(2.f);
    m_helpText.emplace(*font,
                       toSfUtf8String(u8"A / D 또는 방향키: 이동    Space: 점프    Shift: 대시    마우스: 공격\n\n"
                                      u8"자유롭게 조작을 연습하세요. 준비되면 Enter 키를 눌러 던전으로 이동합니다."),
                       19);
    m_helpText->setFillColor(sf::Color(235, 229, 255));
    m_helpText->setOutlineColor(sf::Color(25, 18, 40));
    m_helpText->setOutlineThickness(1.5f);
    return true;
}

void VillageScene::update(float dt) {
    if (!isReady()) {
        return;
    }

    Player *player = m_gameplay.getPlayer();
    Camera *camera = m_gameplay.getCamera();
    m_tileMap->update(dt);
    // 마우스 화면 좌표를 월드 좌표로 변환하기 전에 현재 카메라 뷰를 적용합니다.
    // 기본 뷰를 사용하면 플레이어 중심과 마우스 좌표의 기준 공간이 달라집니다.
    m_window.setView(camera->getView());
    player->update(dt, m_window, *m_tileMap);
    Collision::resolveMapCollision(*player, *m_tileMap, player->ignoresOneWayPlatforms());
    camera->update(player->getPosition());
}

void VillageScene::render() {
    if (!isReady() || !m_titleText || !m_helpText) {
        return;
    }

    Camera *camera = m_gameplay.getCamera();
    Player *player = m_gameplay.getPlayer();
    m_window.setView(camera->getView());
    m_window.draw(*m_tileMap);
    player->render(m_window);
    m_window.setView(m_window.getDefaultView());
    drawCentered(m_window, *m_titleText, {m_window.getSize().x * 0.5f, 42.f});
    drawCentered(m_window, *m_helpText, {m_window.getSize().x * 0.5f, 630.f});
}

bool VillageScene::isReady() const { return m_tileMap && m_gameplay.isReady(); }
