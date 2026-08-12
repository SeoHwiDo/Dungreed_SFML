#include "GameDataManager.h"

#include "LogManager.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace {
std::string makeGameDataPath(std::string_view fileName) { return (std::filesystem::path(__FILE__).parent_path() / "Resources" / "data" / std::string(fileName)).string(); }

WeaponType parseWeaponType(const std::string &value) { return value == "Ranged" ? WeaponType::Ranged : WeaponType::Melee; }

ProjectileTarget parseTarget(const std::string &value) { return value == "Player" ? ProjectileTarget::Player : ProjectileTarget::Monster; }

RoomType parseRoomType(const std::string &value) {
    if (value == "Monster") {
        return RoomType::Monster;
    }
    if (value == "Hut") {
        return RoomType::Hut;
    }
    if (value == "Boss") {
        return RoomType::Boss;
    }
    return RoomType::Start;
}

DoorPosition parseDoorPosition(const std::string &value) {
    if (value == "Up") {
        return DoorPosition::Up;
    }
    if (value == "Down") {
        return DoorPosition::Down;
    }
    if (value == "Left") {
        return DoorPosition::Left;
    }
    return DoorPosition::Right;
}

FairyRewardSize parseFairyRewardSize(const std::string &value) {
    if (value == "M") {
        return FairyRewardSize::Medium;
    }
    if (value == "L") {
        return FairyRewardSize::Large;
    }
    if (value == "XL") {
        return FairyRewardSize::ExtraLarge;
    }
    return FairyRewardSize::Small;
}

/// 방 레퍼런스의 자유 배치 장식 정보를 읽습니다. 잘못된 좌표 배열 항목은 건너뜁니다.
std::vector<DecorativeTileConfig> parseDecorations(const json &decorationsJson) {
    std::vector<DecorativeTileConfig> decorations;
    for (const auto &decorationJson : decorationsJson) {
        DecorativeTileConfig decoration;
        decoration.atlasKey = decorationJson.value("atlasKey", "TileMap");
        decoration.frameName = decorationJson.value("frame", "");
        decoration.animationName = decorationJson.value("animation", "");
        decoration.frameDuration = decorationJson.value("frameDuration", 0.15f);
        decoration.isLoop = decorationJson.value("isLoop", true);
        decoration.drawAboveTiles = decorationJson.value("drawAboveTiles", false);

        const std::vector<float> position = decorationJson.value("position", std::vector<float>{0.f, 0.f});
        const std::vector<float> offset = decorationJson.value("offset", std::vector<float>{0.f, 0.f});
        const std::vector<float> scale = decorationJson.value("scale", std::vector<float>{1.f, 1.f});
        if (position.size() != 2 || offset.size() != 2 || scale.size() != 2) {
            continue;
        }

        decoration.position = {position[0], position[1]};
        decoration.offset = {offset[0], offset[1]};
        decoration.scale = {scale[0], scale[1]};
        decorations.push_back(std::move(decoration));
    }
    return decorations;
}

RoomLayout createStyledRoomLayout(const json &layoutJson) {
    const unsigned int width = layoutJson.at("width").get<unsigned int>();
    const unsigned int height = layoutJson.at("height").get<unsigned int>();
    const unsigned int outlineWidth = layoutJson.value("outlineWidth", 2u);
    const json passageJson = layoutJson.value("passage", json::object());
    const unsigned int topBottomPassageWidth = passageJson.value("topBottomWidth", 3u);
    const unsigned int sidePassageHeight = passageJson.value("sideHeight", 3u);

    if (outlineWidth == 0 || topBottomPassageWidth == 0 || sidePassageHeight == 0 || width < outlineWidth * 2 + topBottomPassageWidth || height < outlineWidth * 2 + sidePassageHeight + 1) {
        return {};
    }

    RoomLayout layout{width, height, std::vector<RoomCell>(width * height, RoomCell::Empty)};
    layout.outlineWidth = outlineWidth;
    layout.topBottomPassageWidth = topBottomPassageWidth;
    layout.sidePassageHeight = sidePassageHeight;

    const auto setCell = [&layout, width](unsigned int x, unsigned int y, RoomCell cell) { layout.cells[x + y * width] = cell; };

    const unsigned int left = outlineWidth;
    const unsigned int right = width - outlineWidth - 1;
    const unsigned int top = outlineWidth;
    const unsigned int ground = height - outlineWidth - 1;

    for (unsigned int x = left; x <= right; ++x) {
        setCell(x, top, RoomCell::Ceiling);
        setCell(x, ground, RoomCell::Ground);
    }
    setCell(left, top, RoomCell::TopLeftCorner);
    setCell(right, top, RoomCell::TopRightCorner);
    setCell(left, ground, RoomCell::BottomLeftCorner);
    setCell(right, ground, RoomCell::BottomRightCorner);

    for (unsigned int y = top + 1; y < ground; ++y) {
        setCell(left, y, RoomCell::LeftWall);
        setCell(right, y, RoomCell::RightWall);
        for (unsigned int x = left + 1; x < right; ++x) {
            setCell(x, y, RoomCell::BackTile);
        }
    }

    layout.playerSpawnCell = sf::Vector2u{left + 4, ground - 1};

    const json platformsJson = layoutJson.value("platforms", json::array());
    for (const auto &platform : platformsJson) {
        const unsigned int platformX = platform.at("x").get<unsigned int>();
        const unsigned int platformY = platform.at("y").get<unsigned int>();
        const unsigned int platformLength = platform.at("length").get<unsigned int>();

        for (unsigned int offset = 0; offset < platformLength; ++offset) {
            const unsigned int x = platformX + offset;
            if (x < width && platformY < height && layout.cells[x + platformY * width] == RoomCell::BackTile) {
                setCell(x, platformY, RoomCell::Platform);
            }
        }
    }

    return layout;
}

// 시작마을은 타일 벽을 그리지 않습니다. 바깥 한 줄의 투명 충돌체만 남겨
// 플레이어가 배경 이미지 바깥으로 나가지 않도록 합니다.
RoomLayout createOpenVillageLayout(const json &layoutJson) {
    const unsigned int width = layoutJson.at("width").get<unsigned int>();
    const unsigned int height = layoutJson.at("height").get<unsigned int>();
    if (width < 3 || height < 3) {
        return {};
    }

    RoomLayout layout{width, height, std::vector<RoomCell>(width * height, RoomCell::OpenSpace)};
    const auto setCell = [&layout, width](unsigned int x, unsigned int y, RoomCell cell) { layout.cells[x + y * width] = cell; };
    for (unsigned int x = 0; x < width; ++x) {
        setCell(x, 0, RoomCell::InvisibleWall);
        setCell(x, height - 1, RoomCell::Ground);
    }
    for (unsigned int y = 1; y + 1 < height; ++y) {
        setCell(0, y, RoomCell::InvisibleWall);
        setCell(width - 1, y, RoomCell::InvisibleWall);
    }

    const std::vector<unsigned int> spawn = layoutJson.value("spawn", std::vector<unsigned int>{width / 2, height - 2});
    if (spawn.size() == 2 && spawn[0] > 0 && spawn[0] + 1 < width && spawn[1] > 0 && spawn[1] + 1 < height) {
        layout.playerSpawnCell = sf::Vector2u{spawn[0], spawn[1]};
    }
    return layout;
}
} // namespace

bool GameDataManager::loadSharedGameData() {
    return loadWeaponsFromFile(makeGameDataPath(kWeaponsDataFileName)) &&
        loadActorDataFromFile(makeGameDataPath(kActorDataFileName));
}

bool GameDataManager::loadVillageData() { return loadSharedGameData() && loadRoomDataFromFile(makeGameDataPath(kRoomDataFileName)); }

bool GameDataManager::loadDungeonData() { return loadVillageData() && loadMonstersFromFile(makeGameDataPath(kMonstersDataFileName)); }

bool GameDataManager::loadWeaponsFromFile(const std::string &path) {
    std::ifstream file(path);
    if (!file) {
        LogManager::getInstance().error("GameDataManager", "무기 데이터 파일 로드 실패: " + path);
        return false;
    }

    json root;
    try {
        file >> root;
    } catch (const json::exception &exception) {
        LogManager::getInstance().error("GameDataManager", "Failed to parse weapon data JSON: " + path + " (" + exception.what() + ')');
        return false;
    }
    m_weapons.clear();

    try {
        for (const auto &entry : root.at("weapons")) {
            WeaponData data;
            data.id = entry.at("id").get<std::string>();
            data.atlasKey = entry.value("atlasKey", "");
            data.frame = entry.value("frame", "");
            data.stat.damage = entry.at("damage").get<float>();
            data.stat.attackSpeed = entry.at("attackSpeed").get<float>();
            data.stat.range = entry.at("range").get<float>();
            data.stat.type = parseWeaponType(entry.at("type").get<std::string>());

            if (entry.contains("projectile")) {
                const auto &projectile = entry.at("projectile");
                ProjectileConfig config;
                config.animationKey = projectile.at("type").get<std::string>();
                config.target = parseTarget(projectile.at("target").get<std::string>());
                config.speed = projectile.at("speed").get<float>();
                config.damage = projectile.value("damage", data.stat.damage);
                config.count = projectile.at("count").get<unsigned int>();
                config.spreadRadian = projectile.at("spreadRadian").get<float>();
                config.lifetime = projectile.at("lifetime").get<float>();
                config.returnAnimationKey = projectile.value("returnAnimationKey", "");
                config.rotateToDirection = projectile.value("rotateToDirection", false);
                config.rotationOffsetRadian = projectile.value("rotationOffsetRadian", 0.f);
                data.stat.projectile = config;
            }

            if (!m_weapons.emplace(data.id, std::move(data)).second) {
                LogManager::getInstance().error("GameDataManager",
                    "Duplicate weapon id: " + entry.at("id").get<std::string>());
                return false;
            }
        }
    } catch (const json::exception &exception) {
        LogManager::getInstance().error("GameDataManager",
            "Invalid weapon data: " + path + " (" + exception.what() + ')');
        return false;
    }

    if (m_weapons.empty()) {
        LogManager::getInstance().error("GameDataManager", "Weapon data contains no usable entries: " + path);
        return false;
    }

    return true;
}

bool GameDataManager::loadActorDataFromFile(const std::string &path) {
    std::ifstream file(path);
    if (!file) {
        LogManager::getInstance().error("GameDataManager", "Actor data file could not be opened: " + path);
        return false;
    }

    json root;
    try {
        file >> root;

        const json &playerJson = root.at("player");
        const json &presetsJson = playerJson.at("presets");
        const auto parseStatus = [](const json &statusJson) {
            const float maxHp = statusJson.at("maxHp").get<float>();
            return Actor::Status{
                maxHp,
                maxHp,
                statusJson.at("power").get<float>(),
                statusJson.at("dex").get<float>()
            };
        };

        PlayerData playerData;
        playerData.atlasKey = playerJson.at("atlasKey").get<std::string>();
        playerData.defaultWeaponId = playerJson.at("defaultWeaponId").get<std::string>();
        playerData.defaultStatus = parseStatus(presetsJson.at("default"));
        playerData.easyStatus = parseStatus(presetsJson.at("easy"));
        if (playerData.atlasKey.empty() || playerData.defaultWeaponId.empty() ||
            !findWeapon(playerData.defaultWeaponId)) {
            LogManager::getInstance().error("GameDataManager",
                "Player data has an invalid atlas or default weapon reference.");
            return false;
        }

        m_bosses.clear();
        for (const json &bossJson : root.at("bosses")) {
            BossData boss;
            boss.id = bossJson.at("id").get<std::string>();
            boss.displayName = bossJson.at("displayName").get<std::string>();
            boss.atlasKey = bossJson.at("atlasKey").get<std::string>();
            boss.status = parseStatus(bossJson.at("status"));
            const json &weaponIds = bossJson.at("patternWeaponIds");
            boss.handLaserWeaponId = weaponIds.at("handLaser").get<std::string>();
            boss.rotatingBulletWeaponId = weaponIds.at("rotatingBullet").get<std::string>();
            boss.swordFanWeaponId = weaponIds.at("swordFan").get<std::string>();
            if (boss.id.empty() || boss.displayName.empty() || boss.atlasKey.empty() ||
                !findWeapon(boss.handLaserWeaponId) || !findWeapon(boss.rotatingBulletWeaponId) ||
                !findWeapon(boss.swordFanWeaponId) || !m_bosses.emplace(boss.id, std::move(boss)).second) {
                LogManager::getInstance().error("GameDataManager",
                    "Boss data has an invalid or duplicate id/reference.");
                return false;
            }
        }

        if (m_bosses.empty()) {
            LogManager::getInstance().error("GameDataManager", "Actor data contains no boss entries: " + path);
            return false;
        }
        m_playerData = std::move(playerData);
        return true;
    } catch (const json::exception &exception) {
        LogManager::getInstance().error("GameDataManager",
            "Invalid actor data: " + path + " (" + exception.what() + ')');
        return false;
    }
}

bool GameDataManager::loadMonstersFromFile(const std::string &path) {
    std::ifstream file(path);
    if (!file) {
        LogManager::getInstance().error("GameDataManager", "몬스터 데이터 파일 로드 실패: " + path);
        return false;
    }

    json root;
    try {
        file >> root;
    } catch (const json::exception &exception) {
        LogManager::getInstance().error("GameDataManager", "Failed to parse monster data JSON: " + path + " (" + exception.what() + ')');
        return false;
    }
    m_monsters.clear();

    try {
    for (const auto &entry : root.at("monsters")) {
        MonsterData data;
        data.id = entry.at("id").get<std::string>();
        data.enabled = entry.at("enabled").get<bool>();
        data.atlasKey = entry.at("atlasKey").get<std::string>();

        const auto &status = entry.at("status");
        data.status.maxHp = status.at("maxHp").get<float>();
        data.status.tmpHp = data.status.maxHp;
        data.status.power = status.at("power").get<float>();
        data.status.dex = status.at("dex").get<float>();

        const auto &ai = entry.at("ai");
        data.behavior.detectRange = ai.at("detectRange").get<float>();
        data.behavior.attackRange = ai.at("attackRange").get<float>();
        data.behavior.idleDuration = ai.at("idleDuration").get<float>();
        data.behavior.patrolDuration = ai.at("patrolDuration").get<float>();
        data.behavior.patrolSpeedMultiplier = ai.at("patrolSpeedMultiplier").get<float>();
        data.behavior.chaseExitRangeMultiplier = ai.at("chaseExitRangeMultiplier").get<float>();
        data.behavior.attackReadyDuration = ai.at("attackReadyDuration").get<float>();
        data.behavior.attackRecoveryDuration = ai.at("attackRecoveryDuration").get<float>();
        data.behavior.attackActiveTimeout = ai.at("attackActiveTimeout").get<float>();
        data.behavior.lockAttackFacing = ai.at("lockAttackFacing").get<bool>();
        data.behavior.playReleaseAnimation = ai.at("playReleaseAnimation").get<bool>();
        data.behavior.waitForAttackCooldownAfterRelease = ai.at("waitForAttackCooldownAfterRelease").get<bool>();

        if (entry.contains("chargeCombo")) {
            const json &chargeCombo = entry.at("chargeCombo");
            data.behavior.chargeCombo = ChargeComboConfig{
                chargeCombo.at("windup").get<float>(),
                chargeCombo.at("duration").get<float>(),
                chargeCombo.at("speedMultiplier").get<float>(),
                chargeCombo.at("stunDuration").get<float>()
            };
        }

        const auto &movement = entry.at("movement");
        data.behavior.moveSpeed = movement.at("moveSpeed").get<float>();
        data.behavior.gravity = movement.at("gravity").get<float>();
        data.behavior.isFlying = movement.at("mode").get<std::string>() == "Flying";
        data.behavior.ignoresWalls = movement.at("ignoresWalls").get<bool>();

        for (const auto &[stateName, animation] : entry.at("animations").items()) {
            MonsterAnimationConfig config;
            config.isLoop = animation.at("isLoop").get<bool>();
            config.frameDuration = animation.at("frameDuration").get<float>();
            data.behavior.animations.emplace(stateName, config);
        }

        data.weaponId = entry.at("weaponId").get<std::string>();
        if (!m_monsters.emplace(data.id, std::move(data)).second) {
            LogManager::getInstance().error("GameDataManager",
                "Duplicate monster id: " + entry.at("id").get<std::string>());
            return false;
        }
    }
    } catch (const json::exception &exception) {
        LogManager::getInstance().error("GameDataManager",
            "Invalid monster data: " + path + " (" + exception.what() + ')');
        return false;
    }

    if (m_monsters.empty()) {
        LogManager::getInstance().error("GameDataManager", "Monster data contains no usable entries: " + path);
        return false;
    }

    return true;
}

bool GameDataManager::loadRoomDataFromFile(const std::string &path) {
    std::ifstream file(path);
    if (!file) {
        LogManager::getInstance().error("GameDataManager", "방 데이터 파일 로드 실패: " + path);
        return false;
    }

    json root;
    try {
        file >> root;
    } catch (const json::exception &exception) {
        LogManager::getInstance().error("GameDataManager", "Failed to parse room data JSON: " + path + " (" + exception.what() + ')');
        return false;
    }
    m_floors.clear();

    for (const auto &floorJson : root.at("floors")) {
        FloorData floor;
        floor.id = floorJson.at("id").get<std::string>();
        floor.name = floorJson.value("name", floor.id);

        for (const auto &referenceJson : floorJson.at("roomReferences")) {
            RoomReferenceData reference;
            reference.id = referenceJson.at("id").get<std::string>();
            reference.type = parseRoomType(referenceJson.value("roomType", "Start"));
            const json &layoutJson = referenceJson.at("layout");
            const std::string generator = layoutJson.value("generator", "StyledRoom");
            reference.layout = generator == "OpenVillage" ? createOpenVillageLayout(layoutJson) : createStyledRoomLayout(layoutJson);
            // 장식은 레퍼런스 최상위와 layout 내부 형식을 모두 허용합니다.
            // 레이아웃과 함께 관리하기 쉬운 layout.decorations를 우선 사용합니다.
            if (layoutJson.contains("decorations")) {
                reference.decorations = parseDecorations(layoutJson.at("decorations"));
            } else {
                const json decorationsJson = referenceJson.value("decorations", json::array());
                reference.decorations = parseDecorations(decorationsJson);
            }
            const json backgroundsJson = referenceJson.value("backgrounds", json::array());
            for (const auto &backgroundJson : backgroundsJson) {
                BackgroundLayerConfig background;
                background.atlasKey = backgroundJson.value("atlasKey", "");
                background.frameName = backgroundJson.value("frame", "");
                background.fitToMap = backgroundJson.value("fitToMap", true);
                if (!background.atlasKey.empty() && !background.frameName.empty()) {
                    reference.backgroundLayers.push_back(std::move(background));
                }
            }
            if (reference.layout.cells.empty()) {
                LogManager::getInstance().error("GameDataManager", "유효한 레이아웃을 만들지 못했습니다: " + reference.id);
                return false;
            }
            floor.roomReferences.emplace(reference.id, std::move(reference));
        }

        for (const auto &roomJson : floorJson.at("rooms")) {
            RoomInstanceData room;
            room.id = roomJson.at("id").get<std::string>();
            room.roomReferenceId = roomJson.at("roomReferenceId").get<std::string>();
            room.role = roomJson.value("role", "");
            room.minDoorCount = roomJson.value("minDoorCount", 0);
            room.maxDoorCount = roomJson.value("maxDoorCount", 0);

            const json availableDoorsJson = roomJson.value("availableDoorPositions", json::array());
            for (const auto &door : availableDoorsJson) {
                room.availableDoorPositions.push_back(parseDoorPosition(door.get<std::string>()));
            }

            const json monsterSpawnsJson = roomJson.value("monsters", json::array());
            for (const auto &spawn : monsterSpawnsJson) {
                RoomMonsterSpawn monsterSpawn;
                monsterSpawn.monsterId = spawn.at("monsterId").get<std::string>();
                monsterSpawn.activationDelay = spawn.value("activationDelay", 1.f);

                const std::vector<float> positionOffset = spawn.value("positionOffset", std::vector<float>{0.f, 0.f});
                if (positionOffset.size() == 2) {
                    monsterSpawn.positionOffset = {positionOffset[0], positionOffset[1]};
                }

                room.monsterSpawns.push_back(monsterSpawn);
            }

            if (roomJson.contains("monster_pool")) {
                room.monsterPhaseConfig.phaseDelay = roomJson.value("phaseDelay", 1.2f);
                room.monsterPhaseConfig.activationDelay = roomJson.value("activationDelay", 0.9f);
                for (const auto &phase : roomJson.at("monster_pool")) {
                    std::vector<RoomMonsterPhaseConfig::MonsterCount> phaseEntries;
                    for (const auto &entry : phase) {
                        phaseEntries.push_back({entry.at("monsterId").get<std::string>(), entry.value("count", 0)});
                    }
                    room.monsterPhaseConfig.monsterPool.push_back(std::move(phaseEntries));
                }
            }

            if (roomJson.contains("clearReward")) {
                const json &rewardJson = roomJson.at("clearReward");
                room.clearReward.enabled = true;
                room.clearReward.fairySize = parseFairyRewardSize(rewardJson.value("fairy", "S"));
                room.clearReward.maxHpIncrease = rewardJson.value("maxHpIncrease", 0.f);
                room.clearReward.powerIncrease = rewardJson.value("powerIncrease", 0.f);
            }

            floor.rooms.emplace(room.id, std::move(room));
        }

        const auto &connection = floorJson.at("connectionGeneration");
        floor.startRoomId = connection.at("startRoomId").get<std::string>();
        floor.bossRoomId = connection.value("bossRoomId", "");
        floor.randomMonsterRoomCount = connection.value("randomMonsterRoomCount", 0u);
        const json shuffleRoomIdsJson = connection.value("shuffleRoomIds", json::array());
        for (const auto &roomId : shuffleRoomIdsJson) {
            floor.shuffleRoomIds.push_back(roomId.get<std::string>());
        }

        m_floors.emplace(floor.id, std::move(floor));
    }

    if (m_floors.empty()) {
        LogManager::getInstance().error("GameDataManager", "방 데이터에 유효한 층 정보가 없습니다: " + path);
        return false;
    }
    return true;
}

const WeaponData *GameDataManager::findWeapon(const std::string &id) const {
    const auto it = m_weapons.find(id);
    return it == m_weapons.end() ? nullptr : &it->second;
}

const MonsterData *GameDataManager::findMonster(const std::string &id) const {
    const auto it = m_monsters.find(id);
    return it == m_monsters.end() ? nullptr : &it->second;
}

const FloorData *GameDataManager::findFloor(const std::string &id) const {
    const auto it = m_floors.find(id);
    return it == m_floors.end() ? nullptr : &it->second;
}

const PlayerData *GameDataManager::getPlayerData() const {
    return m_playerData ? &*m_playerData : nullptr;
}

const BossData *GameDataManager::findBoss(const std::string &id) const {
    const auto it = m_bosses.find(id);
    return it == m_bosses.end() ? nullptr : &it->second;
}

PoolPrewarmPlan GameDataManager::createPoolPrewarmPlan(float reserveRatio) const {
    const float safeReserveRatio = std::max(0.f, reserveRatio);
    const auto addReserve = [safeReserveRatio](std::size_t count) { return count == 0 ? std::size_t{0} : static_cast<std::size_t>(std::ceil(count * (1.f + safeReserveRatio))); };

    std::unordered_map<std::string, std::size_t> monsterCounts;
    std::size_t projectileSpawnCount = 0;
    for (const auto &[floorId, floor] : m_floors) {
        for (const auto &[roomId, room] : floor.rooms) {
            if (room.monsterPhaseConfig.isEnabled()) {
                for (const auto &phase : room.monsterPhaseConfig.monsterPool) {
                    std::unordered_map<std::string, std::size_t> phaseMonsterCounts;
                    std::size_t phaseProjectileCount = 0;
                    for (const RoomMonsterPhaseConfig::MonsterCount &entry : phase) {
                        const MonsterData *monsterData = findMonster(entry.monsterId);
                        if (!monsterData || !monsterData->enabled || entry.count <= 0) {
                            continue;
                        }

                        const std::size_t count = static_cast<std::size_t>(entry.count);
                        phaseMonsterCounts[monsterData->id] += count;
                        const WeaponData *weaponData = findWeapon(monsterData->weaponId);
                        if (weaponData && weaponData->stat.projectile) {
                            phaseProjectileCount += count * weaponData->stat.projectile->count;
                        }
                    }

                    for (const auto &[monsterId, count] : phaseMonsterCounts) {
                        monsterCounts[monsterId] = std::max(monsterCounts[monsterId], count);
                    }
                    projectileSpawnCount = std::max(projectileSpawnCount, phaseProjectileCount);
                }
                continue;
            }

            for (const RoomMonsterSpawn &spawn : room.monsterSpawns) {
                const MonsterData *monsterData = findMonster(spawn.monsterId);
                if (!monsterData || !monsterData->enabled) {
                    continue;
                }

                ++monsterCounts[monsterData->id];
                const WeaponData *weaponData = findWeapon(monsterData->weaponId);
                if (weaponData && weaponData->stat.projectile) {
                    projectileSpawnCount += weaponData->stat.projectile->count;
                }
            }
        }
    }

    PoolPrewarmPlan plan;
    for (const auto &[monsterId, count] : monsterCounts) {
        const MonsterData *monsterData = findMonster(monsterId);
        if (!monsterData) {
            continue;
        }

        plan.monsters.push_back({monsterData->id, monsterData->status, monsterData->atlasKey, monsterData->behavior, addReserve(count)});
    }

    std::size_t configuredProjectileBurstCount = 0;
    for (const auto &[weaponId, weapon] : m_weapons) {
        if (weapon.stat.projectile) {
            configuredProjectileBurstCount += weapon.stat.projectile->count;
        }
    }
    plan.projectileCount = addReserve(std::max(projectileSpawnCount, configuredProjectileBurstCount));
    return plan;
}
std::shared_ptr<Equip> GameDataManager::createEquip(const std::string &weaponId) const {
    const WeaponData *data = findWeapon(weaponId);
    if (!data) {
        return nullptr;
    }

    auto equip = std::make_shared<Equip>(data->id, data->stat);
    if (!data->atlasKey.empty() && !data->frame.empty()) {
        equip->init(data->atlasKey, data->frame);
    }

    return equip;
}
