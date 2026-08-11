#pragma once

#include "Monster.h"
#include "Room.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct WeaponData {
    std::string id;
    std::string atlasKey;
    std::string frame;
    EquipStat stat;
};

struct MonsterData {
    std::string id;
    bool enabled = true;
    std::string atlasKey = "Monster";
    Actor::Status status{ kDefaultMaxHp, kDefaultMaxHp, kDefaultPower, kDefaultDex };
    MonsterBehaviorConfig behavior;
    std::string weaponId;
};

struct RoomReferenceData {
    std::string id;
    RoomType type = RoomType::Start;
    RoomLayout layout;
    std::vector<DecorativeTileConfig> decorations;
    std::vector<BackgroundLayerConfig> backgroundLayers;
};

struct RoomInstanceData {
    std::string id;
    std::string roomReferenceId;
    std::string role;
    std::vector<DoorPosition> availableDoorPositions;
    int minDoorCount = 0;
    int maxDoorCount = 0;
    std::vector<RoomMonsterSpawn> monsterSpawns;
    RoomMonsterPhaseConfig monsterPhaseConfig;
    RoomClearRewardConfig clearReward;
};

struct FloorData {
    std::string id;
    std::string name;
    std::unordered_map<std::string, RoomReferenceData> roomReferences;
    std::unordered_map<std::string, RoomInstanceData> rooms;
    std::string startRoomId;
    std::string bossRoomId;
    std::vector<std::string> shuffleRoomIds;
    /// 0이면 모든 몬스터방을 사용하고, 양수면 그 수만큼 무작위로 선택합니다.
    std::size_t randomMonsterRoomCount = 0;
};
struct MonsterPrewarmData {
    std::string monsterId;
    Actor::Status status;
    std::string atlasKey;
    MonsterBehaviorConfig behavior;
    std::size_t count = 0;
};

struct PoolPrewarmPlan {
    std::vector<MonsterPrewarmData> monsters;
    std::size_t projectileCount = 0;
};

class GameDataManager {
public:
    static GameDataManager& getInstance() {
        static GameDataManager instance;
        return instance;
    }

    GameDataManager(const GameDataManager&) = delete;
    GameDataManager& operator=(const GameDataManager&) = delete;
    bool loadWeapons(const std::string& path);
    bool loadMonsters(const std::string& path);
    bool loadRoomData(const std::string& path);

    const WeaponData* findWeapon(const std::string& id) const;
    const MonsterData* findMonster(const std::string& id) const;
    const FloorData* findFloor(const std::string& id) const;
    PoolPrewarmPlan createPoolPrewarmPlan(float reserveRatio) const;

    std::shared_ptr<Equip> createEquip(const std::string& weaponId) const;

private:
    GameDataManager() = default;
    ~GameDataManager() = default;
    std::unordered_map<std::string, WeaponData> m_weapons;
    std::unordered_map<std::string, MonsterData> m_monsters;
    std::unordered_map<std::string, FloorData> m_floors;
};
