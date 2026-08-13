#include "DungeonScene.h"

#include "Camera.h"
#include "AudioManager.h"
#include "Collision.h"
#include "CombatManager.h"
#include "DebugManager.h"
#include "EffectManager.h"
#include "GameDataManager.h"
#include "GameplayContext.h"
#include "LogManager.h"
#include "MapManager.h"
#include "MonsterManager.h"
#include "ObjectPoolingManager.h"
#include "Player.h"
#include "ResourceManager.h"
#include "RewardChestManager.h"
#include "Room.h"
#include "SkelBoss.h"
#include "TileMap.h"
#include "UIManager.h"

#include <iomanip>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <sstream>
#include <unordered_set>

namespace {
constexpr float kGameplayCameraZoom = 3.5f;
constexpr float kBossCinematicCameraZoom = 5.2f;
constexpr std::size_t kBossProjectilePoolCapacity = 384;

RoomTileSet createRoomTileSet() { return {"Wall_Outter.png", "Wall_Top.png", "Wall_Ground.png", "Wall_Left.png", "Wall_Right.png", "Wall_H0.png", "Wall_H2.png", "Wall_H6.png", "Wall_H8.png", "Wall_TopLCorner.png", "Wall_TopRCorner.png", "Wall_BotLCorner.png", "Wall_BotRCorner.png", "Back_Inner.png", "Back_Top.png", "Back_Ground.png", "Back_Left.png", "Back_Right.png", "Back_TopLcorner.png", "Back_TopRCorner.png", "BackBotLCorner.png", "Back_BotRCorner.png", "Back_DoorTopL.png", "Back_DoorTopR.png", "Back_DoorBotL.png", "Back_DoorBotR.png", "Platform.png"}; }

std::string makeFloorId(unsigned int floorNumber) {
    std::ostringstream stream;
    stream << "floor_" << std::setw(2) << std::setfill('0') << floorNumber;
    return stream.str();
}
} // namespace

DungeonScene::DungeonScene(sf::RenderWindow &window, GameplayContext &gameplay) : m_window(window), m_gameplay(gameplay) {}

DungeonScene::~DungeonScene() = default;

bool DungeonScene::enter(unsigned int floorNumber) {
    if (floorNumber == 0) {
        LogManager::getInstance().error("DungeonScene", "던전 층 번호는 1 이상이어야 합니다.");
        return false;
    }

    leave();

    auto &resources = ResourceManager::getInstance();
    auto &gameData = GameDataManager::getInstance();
    auto &mapManager = MapManager::getInstance();
    auto &objectPool = ObjectPoolingManager::getInstance();
    auto &rewardChestManager = RewardChestManager::getInstance();
    auto &uiManager = UIManager::getInstance();
    if (!resources.loadDungeonResources() || !gameData.loadDungeonData()) {
        LogManager::getInstance().error("DungeonScene", "던전 리소스 또는 게임 데이터 로드에 실패했습니다.");
        return false;
    }

    const FloorData *floor = gameData.findFloor(makeFloorId(floorNumber));
    const RoomTileSet roomTiles = createRoomTileSet();
    if (!floor || !mapManager.createCurrentRoomFromData(*floor, floor->startRoomId) || !mapManager.preloadFloorTileMaps("TileMap", roomTiles) || !placePlayerAtCurrentRoom()) {
        LogManager::getInstance().error("DungeonScene", "던전 시작 방 또는 타일맵 초기화에 실패했습니다.");
        return false;
    }

    objectPool.prewarmFromGameData(gameData);
    objectPool.prewarmProjectiles(kBossProjectilePoolCapacity);
    if (!rewardChestManager.init() || !uiManager.init(m_window)) {
        LogManager::getInstance().error("DungeonScene", "던전 UI 또는 보상 상자 초기화에 실패했습니다.");
        return false;
    }

    m_areMonstersActivated = false;
    m_bossDefeated = false;
    m_floorNumber = floorNumber;
    AudioManager::getInstance().playBgm("Dungeon_BGM");
    return true;
}

void DungeonScene::leave() {
    auto &objectPool = ObjectPoolingManager::getInstance();
    MonsterManager::getInstance().clearActiveRoom(objectPool);
    EffectManager::getInstance().clear(objectPool);
    objectPool.clearActiveProjectiles();
    RewardChestManager::getInstance().reset();
    m_activeBoss.reset();
    m_areMonstersActivated = false;
    m_bossDefeated = false;
}

void DungeonScene::update(float dt) {
    Player *player = m_gameplay.getPlayer();
    Camera *camera = m_gameplay.getCamera();
    if (!isReady() || !player || !camera) {
        return;
    }

    auto &mapManager = MapManager::getInstance();
    auto &objectPool = ObjectPoolingManager::getInstance();
    auto &monsterManager = MonsterManager::getInstance();
    auto &combatManager = CombatManager::getInstance();
    auto &effectManager = EffectManager::getInstance();
    auto &rewardChestManager = RewardChestManager::getInstance();
    auto &gameData = GameDataManager::getInstance();
    auto &uiManager = UIManager::getInstance();

    TileMap *activeTileMap = mapManager.getCurrentTileMap();
    Room *currentRoom = mapManager.getCurrentRoom();
    if (!activeTileMap || !currentRoom) {
        return;
    }

    TileMap &tileMap = *activeTileMap;
    const bool isBossRoom = currentRoom->getInfo().type == RoomType::Boss;
    // 같은 씬 BGM 요청은 AudioManager가 무시하므로 매 프레임 호출해도 재시작되지 않습니다.
    AudioManager::getInstance().playBgm(isBossRoom ? "Boss_BGM" : "Dungeon_BGM");
    if (isBossRoom && !currentRoom->getInfo().isClear && !m_activeBoss) {
        const BossData *bossData = GameDataManager::getInstance().findBoss("SkelBoss");
        if (!bossData) {
            LogManager::getInstance().error("DungeonScene",
                "Critical boss data is missing: SkelBoss. Terminating game.");
            std::exit(EXIT_FAILURE);
        }
        m_activeBoss = std::make_unique<SkelBoss>(*bossData);
        std::cout << "[DungeonScene] SkelBoss created\n";
        m_activeBoss->placeAtMapCenter(tileMap);
    } else if (!isBossRoom) {
        m_activeBoss.reset();
    }

    effectManager.update(dt, objectPool);
    const bool isFirstRoomActivation = !m_areMonstersActivated;
    if (isFirstRoomActivation) {
        if (!isBossRoom) {
            mapManager.requestCurrentRoomMonsters(monsterManager, objectPool, gameData, tileMap, player->getBodyCenterPosition(), effectManager);
        }
        m_areMonstersActivated = true;
    }

    // 방에 들어온 프레임에 조우 상태를 먼저 확정해 닫힌 문 충돌체를 이동 전에 추가합니다.
    currentRoom->setTraversalLocked(isBossRoom && m_activeBoss && !m_activeBoss->dead());
    tileMap.setDoorsLocked(currentRoom->isTraversalLocked());

    m_window.setView(camera->getView());
    player->update(dt, m_window, tileMap);

    if (!isFirstRoomActivation && !isBossRoom) {
        monsterManager.update(dt, *player, objectPool, tileMap, effectManager);
    }
    Collision::resolveMapCollision(*player, tileMap, player->ignoresOneWayPlatforms());

    if (m_activeBoss) {
        m_activeBoss->update(dt, *player, objectPool, effectManager, tileMap);
        if (m_activeBoss->dead()) {
            currentRoom->setClear(true);
            m_bossDefeated = true;
        }
    }

    currentRoom->setTraversalLocked(isBossRoom && m_activeBoss && !m_activeBoss->dead());
    tileMap.setDoorsLocked(currentRoom->isTraversalLocked());
    tileMap.update(dt);

    bool didChangeRoom = false;
    if (const auto enteredDoor = currentRoom->getEnteredDoor(player->getGlobalBounds(), player->getPreviousGlobalBounds(), tileMap); enteredDoor && mapManager.moveCurrentRoom(*enteredDoor)) {
        monsterManager.clearActiveRoom(objectPool);
        effectManager.clear(objectPool);
        player->cancelDash();
        placePlayerAtCurrentRoom();
        m_activeBoss.reset();
        m_areMonstersActivated = false;
        didChangeRoom = true;
    }

    if (!didChangeRoom) {
        std::unordered_set<EntityId> playerHitMonsters = combatManager.resolvePlayerAttack(*player, objectPool, effectManager, m_activeBoss.get());
        const auto projectileHits = combatManager.updateProjectiles(dt, *player, objectPool, tileMap, ProjectileTarget::Monster, m_activeBoss.get());
        playerHitMonsters.insert(projectileHits.begin(), projectileHits.end());
        combatManager.resolveMonsterAttacks(dt, *player, objectPool, tileMap, playerHitMonsters);
        combatManager.updateProjectiles(dt, *player, objectPool, tileMap, ProjectileTarget::Player, m_activeBoss.get());
        rewardChestManager.update(dt, *currentRoom, tileMap, *player, effectManager, objectPool);
    }

    const bool isBossCinematic = m_activeBoss && m_activeBoss->isSummoning();
    camera->setZoom(isBossCinematic ? kBossCinematicCameraZoom : kGameplayCameraZoom);
    camera->update(isBossCinematic ? m_activeBoss->getBodyCenterPosition() : player->getPosition());
    uiManager.update(*player, dt, m_window);
}

void DungeonScene::render() {
    if (!isReady()) {
        return;
    }
    auto& debugManager = DebugManager::getInstance();

    if (debugManager.hasRoomPreviews()) {
        m_window.setView(m_window.getDefaultView());
        debugManager.renderRoomPreviews(m_window);
        return;
    }
    auto &mapManager = MapManager::getInstance();
    auto &objectPool = ObjectPoolingManager::getInstance();
    auto &effectManager = EffectManager::getInstance();
    auto &rewardChestManager = RewardChestManager::getInstance();
    auto &uiManager = UIManager::getInstance();
    
    Camera *camera = m_gameplay.getCamera();
    Player *player = m_gameplay.getPlayer();
    m_window.setView(camera->getView());
    objectPool.renderBehindTiles(m_window);
    if (const TileMap *tileMap = mapManager.getCurrentTileMap()) {
        m_window.draw(*tileMap);
    }
    effectManager.renderBehindActors(m_window, objectPool);
    if (m_activeBoss) {
        m_activeBoss->render(m_window);
    }
    player->render(m_window);
    objectPool.render(m_window);
    effectManager.render(m_window, objectPool);
    rewardChestManager.render(m_window);
    if (m_showCombatBounds) {
        debugManager.renderCombatBounds(m_window, *player, objectPool);
    }

    m_window.setView(m_window.getDefaultView());
    uiManager.render(m_window);
}

bool DungeonScene::spawnDebugRoom(const std::string &floorId, const std::string &roomId) {
    auto &debugManager = DebugManager::getInstance();
    auto &gameData = GameDataManager::getInstance();
    auto &mapManager = MapManager::getInstance();
    auto &monsterManager = MonsterManager::getInstance();
    auto &objectPool = ObjectPoolingManager::getInstance();
    auto &effectManager = EffectManager::getInstance();
    const RoomTileSet roomTiles = createRoomTileSet();

    if (roomId == "all") {
        const FloorData* floor = gameData.findFloor(floorId);
        if (!floor) {
            return false;
        }

        // 실제 층 구성 생성
        if (!mapManager.createCurrentRoomFromData(*floor, floor->startRoomId)) {
            return false;
        }

        // 생성된 층의 Room들을 가져와 전체 프리뷰 생성
        return debugManager.buildRoomPreviews(
            mapManager.getFloorRoomsInDataOrder(),
            "TileMap",
            roomTiles,
            m_window.getSize()
        );
    }
    if (!debugManager.spawnRoom(floorId, roomId, gameData, mapManager, "TileMap", roomTiles)) {
        return false;
    }

    monsterManager.clearActiveRoom(objectPool);
    effectManager.clear(objectPool);
    m_activeBoss.reset();
    m_areMonstersActivated = false;
    return placePlayerAtCurrentRoom();
}

void DungeonScene::toggleCombatBounds() { m_showCombatBounds = !m_showCombatBounds; }

bool DungeonScene::isReady() const { return m_gameplay.isReady(); }

bool DungeonScene::consumeBossDefeat() {
    const bool wasDefeated = m_bossDefeated;
    m_bossDefeated = false;
    return wasDefeated;
}

bool DungeonScene::placePlayerAtCurrentRoom() {
    auto &mapManager = MapManager::getInstance();
    Room *room = mapManager.getCurrentRoom();
    TileMap *tileMap = mapManager.getCurrentTileMap();
    if (!room || !tileMap) {
        return false;
    }

    return m_gameplay.placePlayerAtRoomSpawn(*room, *tileMap, kGameplayCameraZoom);
}
