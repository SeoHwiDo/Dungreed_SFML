#include "GameDataManager.h"

#include <algorithm>
#include <cmath>
#include <fstream>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace {
WeaponType parseWeaponType(const std::string& value) {
    return value == "Ranged" ? WeaponType::Ranged : WeaponType::Melee;
}

ProjectileType parseProjectileType(const std::string& value) {
    if (value == "Fireball") {
        return ProjectileType::Fireball;
    }
    if (value == "Bullet") {
        return ProjectileType::Bullet;
    }
    if (value == "BabyBatBullet") {
        return ProjectileType::BabyBatBullet;
    }
    if (value == "BansheeBullet") {
        return ProjectileType::BansheeBullet;
    }
    return ProjectileType::Arrow;
}

MonsterAttackPattern parseAttackPattern(const std::string& value) {
    if (value == "Ranged") {
        return MonsterAttackPattern::Ranged;
    }
    if (value == "GhostTouch") {
        return MonsterAttackPattern::GhostTouch;
    }
    if (value == "RadialProjectile") {
        return MonsterAttackPattern::RadialProjectile;
    }
    if (value == "ChargeCombo") {
        return MonsterAttackPattern::ChargeCombo;
    }
    return MonsterAttackPattern::Standard;
}

ProjectileTarget parseTarget(const std::string& value) {
    return value == "Player" ? ProjectileTarget::Player : ProjectileTarget::Monster;
}

RoomType parseRoomType(const std::string& value) {
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

DoorPosition parseDoorPosition(const std::string& value) {
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

RoomLayout createStyledRoomLayout(const json& layoutJson) {
    const unsigned int width = layoutJson.at("width").get<unsigned int>();
    const unsigned int height = layoutJson.at("height").get<unsigned int>();
    const unsigned int outlineWidth = layoutJson.value("outlineWidth", 2u);

    if (width <= outlineWidth * 2 || height <= outlineWidth * 2) {
        return {};
    }

    RoomLayout layout{
        width,
        height,
        std::vector<RoomCell>(width * height, RoomCell::Empty)
    };

    const auto setCell = [&layout, width](unsigned int x, unsigned int y, RoomCell cell) {
        layout.cells[x + y * width] = cell;
    };

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

    setCell(left + 4, ground - 1, RoomCell::SpawnPoint);

    for (const auto& platform : layoutJson.value("platforms", json::array())) {
        const unsigned int platformX = platform.at("x").get<unsigned int>();
        const unsigned int platformY = platform.at("y").get<unsigned int>();
        const unsigned int platformLength = platform.at("length").get<unsigned int>();

        for (unsigned int offset = 0; offset < platformLength; ++offset) {
            const unsigned int x = platformX + offset;
            if (x < width && platformY < height &&
                layout.cells[x + platformY * width] == RoomCell::BackTile) {
                setCell(x, platformY, RoomCell::Platform);
            }
        }
    }

    return layout;
}
}

bool GameDataManager::loadWeapons(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        return false;
    }

    json root;
    file >> root;
    m_weapons.clear();

    for (const auto& entry : root.at("weapons")) {
        WeaponData data;
        data.id = entry.at("id").get<std::string>();
        data.atlasKey = entry.value("atlasKey", "");
        data.frame = entry.value("frame", "");
        data.stat.damage = entry.value("damage", 10.f);
        data.stat.attackSpeed = entry.value("attackSpeed", 1.f);
        data.stat.range = entry.value("range", 50.f);
        data.stat.type = parseWeaponType(entry.value("type", "Melee"));

        if (entry.contains("projectile")) {
            const auto& projectile = entry.at("projectile");
            ProjectileConfig config;
            config.type = parseProjectileType(projectile.value("type", "Arrow"));
            config.target = parseTarget(projectile.value("target", "Monster"));
            config.speed = projectile.value("speed", 400.f);
            config.damage = projectile.value("damage", data.stat.damage);
            config.count = projectile.value("count", 1u);
            config.spreadRadian = projectile.value("spreadRadian", 0.f);
            config.lifetime = projectile.value("lifetime", 3.f);
            data.stat.projectile = config;
        }

        m_weapons.emplace(data.id, std::move(data));
    }

    return true;
}

bool GameDataManager::loadMonsters(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        return false;
    }

    json root;
    file >> root;
    m_monsters.clear();

    for (const auto& entry : root.at("monsters")) {
        MonsterData data;
        data.id = entry.at("id").get<std::string>();
        data.enabled = entry.value("enabled", true);
        data.atlasKey = entry.value("atlasKey", "Monster");

        const auto& status = entry.at("status");
        data.status.maxHp = status.value("maxHp", kDefaultMaxHp);
        data.status.tmpHp = data.status.maxHp;
        data.status.power = status.value("power", kDefaultPower);
        data.status.dex = status.value("dex", kDefaultDex);

        const auto& ai = entry.at("ai");
        data.behavior.detectRange = ai.value("detectRange", 100.f);
        data.behavior.attackRange = ai.value("attackRange", 50.f);
        data.behavior.attackWindup = ai.value("attackWindup", 0.35f);
        data.behavior.attackOnLastFrame = ai.value("attackOnLastFrame", true);
        data.behavior.chargeDuration = ai.value("chargeDuration", 0.55f);
        data.behavior.chargeSpeedMultiplier = ai.value("chargeSpeedMultiplier", 3.f);
        data.behavior.stunDuration = ai.value("stunDuration", 0.45f);
        data.behavior.attackPattern = parseAttackPattern(
            ai.value("attackPattern", "Standard"));

        if (entry.contains("movement")) {
            const auto& movement = entry.at("movement");
            data.behavior.moveSpeed = movement.value("moveSpeed", 300.f);
            data.behavior.isFlying = movement.value("mode", "Ground") == "Flying";
            data.behavior.ignoresWalls = movement.value("ignoresWalls", false);
        }

        data.weaponId = entry.value("weaponId", "");
        m_monsters.emplace(data.id, std::move(data));
    }

    return true;
}

bool GameDataManager::loadRoomData(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        return false;
    }

    json root;
    file >> root;
    m_floors.clear();

    for (const auto& floorJson : root.at("floors")) {
        FloorData floor;
        floor.id = floorJson.at("id").get<std::string>();
        floor.name = floorJson.value("name", floor.id);

        for (const auto& referenceJson : floorJson.at("roomReferences")) {
            RoomReferenceData reference;
            reference.id = referenceJson.at("id").get<std::string>();
            reference.type = parseRoomType(referenceJson.value("roomType", "Start"));
            reference.layout = createStyledRoomLayout(referenceJson.at("layout"));
            if (reference.layout.cells.empty()) {
                return false;
            }
            floor.roomReferences.emplace(reference.id, std::move(reference));
        }

        for (const auto& roomJson : floorJson.at("rooms")) {
            RoomInstanceData room;
            room.id = roomJson.at("id").get<std::string>();
            room.roomReferenceId = roomJson.at("roomReferenceId").get<std::string>();
            room.role = roomJson.value("role", "");
            room.minDoorCount = roomJson.value("minDoorCount", 0);
            room.maxDoorCount = roomJson.value("maxDoorCount", 0);

            for (const auto& door : roomJson.value("availableDoorPositions", json::array())) {
                room.availableDoorPositions.push_back(parseDoorPosition(door.get<std::string>()));
            }

            for (const auto& spawn : roomJson.value("monsters", json::array())) {
                RoomMonsterSpawn monsterSpawn;
                monsterSpawn.monsterId = spawn.at("monsterId").get<std::string>();
                monsterSpawn.activationDelay = spawn.value("activationDelay", 1.f);

                const std::vector<float> positionOffset =
                    spawn.value("positionOffset", std::vector<float>{ 0.f, 0.f });
                if (positionOffset.size() == 2) {
                    monsterSpawn.positionOffset = { positionOffset[0], positionOffset[1] };
                }

                room.monsterSpawns.push_back(monsterSpawn);
            }

            if (roomJson.contains("spawnPhases")) {
                const auto& phases = roomJson.at("spawnPhases");
                room.monsterPhaseConfig.minPhaseCount = phases.value("minPhaseCount", 2);
                room.monsterPhaseConfig.maxPhaseCount = phases.value("maxPhaseCount", 3);
                room.monsterPhaseConfig.minMonstersPerPhase =
                    phases.value("minMonstersPerPhase", 3);
                room.monsterPhaseConfig.maxMonstersPerPhase =
                    phases.value("maxMonstersPerPhase", 4);
                room.monsterPhaseConfig.phaseDelay = phases.value("phaseDelay", 1.2f);
                room.monsterPhaseConfig.activationDelay =
                    phases.value("activationDelay", 0.9f);
                for (const auto& monsterId :
                    phases.value("monsterPool", json::array())) {
                    room.monsterPhaseConfig.monsterPool.push_back(
                        monsterId.get<std::string>());
                }
            }

            floor.rooms.emplace(room.id, std::move(room));
        }

        const auto& connection = floorJson.at("connectionGeneration");
        floor.startRoomId = connection.at("startRoomId").get<std::string>();
        floor.bossRoomId = connection.at("bossRoomId").get<std::string>();
        for (const auto& roomId : connection.value("shuffleRoomIds", json::array())) {
            floor.shuffleRoomIds.push_back(roomId.get<std::string>());
        }

        m_floors.emplace(floor.id, std::move(floor));
    }

    return !m_floors.empty();
}

const WeaponData* GameDataManager::findWeapon(const std::string& id) const {
    const auto it = m_weapons.find(id);
    return it == m_weapons.end() ? nullptr : &it->second;
}

const MonsterData* GameDataManager::findMonster(const std::string& id) const {
    const auto it = m_monsters.find(id);
    return it == m_monsters.end() ? nullptr : &it->second;
}

const FloorData* GameDataManager::findFloor(const std::string& id) const {
    const auto it = m_floors.find(id);
    return it == m_floors.end() ? nullptr : &it->second;
}


PoolPrewarmPlan GameDataManager::createPoolPrewarmPlan(float reserveRatio) const {
    const float safeReserveRatio = std::max(0.f, reserveRatio);
    const auto addReserve = [safeReserveRatio](std::size_t count) {
        return count == 0
            ? std::size_t{ 0 }
            : static_cast<std::size_t>(std::ceil(count * (1.f + safeReserveRatio)));
    };

    std::unordered_map<std::string, std::size_t> monsterCounts;
    std::size_t projectileSpawnCount = 0;
    for (const auto& [floorId, floor] : m_floors) {
        for (const auto& [roomId, room] : floor.rooms) {
            if (room.monsterPhaseConfig.isEnabled()) {
                const std::size_t maximumMonsterCount =
                    static_cast<std::size_t>(
                        room.monsterPhaseConfig.maxMonstersPerPhase);
                std::size_t maximumProjectileCount = 0;

                for (const std::string& monsterId :
                    room.monsterPhaseConfig.monsterPool) {
                    const MonsterData* monsterData = findMonster(monsterId);
                    if (!monsterData || !monsterData->enabled) {
                        continue;
                    }

                    monsterCounts[monsterData->id] = std::max(
                        monsterCounts[monsterData->id], maximumMonsterCount);
                    const WeaponData* weaponData =
                        findWeapon(monsterData->weaponId);
                    if (weaponData && weaponData->stat.projectile) {
                        maximumProjectileCount = std::max(
                            maximumProjectileCount,
                            maximumMonsterCount *
                                weaponData->stat.projectile->count);
                    }
                }

                projectileSpawnCount = std::max(
                    projectileSpawnCount, maximumProjectileCount);
                continue;
            }

            for (const RoomMonsterSpawn& spawn : room.monsterSpawns) {
                const MonsterData* monsterData = findMonster(spawn.monsterId);
                if (!monsterData || !monsterData->enabled) {
                    continue;
                }

                ++monsterCounts[monsterData->id];
                const WeaponData* weaponData = findWeapon(monsterData->weaponId);
                if (weaponData && weaponData->stat.projectile) {
                    projectileSpawnCount += weaponData->stat.projectile->count;
                }
            }
        }
    }

    PoolPrewarmPlan plan;
    for (const auto& [monsterId, count] : monsterCounts) {
        const MonsterData* monsterData = findMonster(monsterId);
        if (!monsterData) {
            continue;
        }

        plan.monsters.push_back({
            monsterData->id,
            monsterData->status,
            monsterData->atlasKey,
            monsterData->behavior,
            addReserve(count)
        });
    }

    std::size_t configuredProjectileBurstCount = 0;
    for (const auto& [weaponId, weapon] : m_weapons) {
        if (weapon.stat.projectile) {
            configuredProjectileBurstCount += weapon.stat.projectile->count;
        }
    }
    plan.projectileCount = addReserve(std::max(projectileSpawnCount,
        configuredProjectileBurstCount));
    return plan;
}
std::shared_ptr<Equip> GameDataManager::createEquip(const std::string& weaponId) const {
    const WeaponData* data = findWeapon(weaponId);
    if (!data) {
        return nullptr;
    }

    auto equip = std::make_shared<Equip>(data->id, data->stat);
    if (!data->atlasKey.empty() && !data->frame.empty()) {
        equip->init(data->atlasKey, data->frame);
    }

    return equip;
}