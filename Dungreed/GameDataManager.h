#pragma once

#include "Monster.h"
#include "Room.h"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

inline constexpr std::string_view kWeaponsDataFileName = "weapons.json";
inline constexpr std::string_view kMonstersDataFileName = "monsters.json";
inline constexpr std::string_view kRoomDataFileName = "room_data.json";
inline constexpr std::string_view kActorDataFileName = "actor_data.json";

struct WeaponData {
    std::string id;
    std::string atlasKey;
    std::string frame;
    EquipStat stat;
};

struct MonsterData {
    std::string id;
    bool enabled = true;
    std::string atlasKey;
    Actor::Status status{};
    MonsterBehaviorConfig behavior;
    std::string weaponId;
};

struct PlayerData {
    std::string atlasKey;
    std::string defaultWeaponId;
    Actor::Status defaultStatus{};
    Actor::Status easyStatus{};
};

struct BossData {
    std::string id;
    std::string displayName;
    std::string atlasKey;
    Actor::Status status{};
    std::string handLaserWeaponId;
    std::string rotatingBulletWeaponId;
    std::string swordFanWeaponId;
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
    static GameDataManager &getInstance() {
        static GameDataManager instance;
        return instance;
    }

    GameDataManager(const GameDataManager &) = delete;
    GameDataManager &operator=(const GameDataManager &) = delete;
    /// 공용 플레이어 장비 데이터를 불러옵니다.
    bool loadSharedGameData();
    /// 시작마을에 필요한 장비·방 데이터를 불러옵니다.
    bool loadVillageData();
    /// 던전에 필요한 장비·방·몬스터 데이터를 불러옵니다.
    bool loadDungeonData();

    const WeaponData *findWeapon(const std::string &id) const;
    const MonsterData *findMonster(const std::string &id) const;
    const FloorData *findFloor(const std::string &id) const;
    const PlayerData *getPlayerData() const;
    const BossData *findBoss(const std::string &id) const;
    PoolPrewarmPlan createPoolPrewarmPlan(float reserveRatio) const;

    std::shared_ptr<Equip> createEquip(const std::string &weaponId) const;

  private:
    GameDataManager() = default;
    ~GameDataManager() = default;
    bool loadWeaponsFromFile(const std::string &path);
    bool loadActorDataFromFile(const std::string &path);
    bool loadMonstersFromFile(const std::string &path);
    bool loadRoomDataFromFile(const std::string &path);
    std::unordered_map<std::string, WeaponData> m_weapons;
    std::optional<PlayerData> m_playerData;
    std::unordered_map<std::string, BossData> m_bosses;
    std::unordered_map<std::string, MonsterData> m_monsters;
    std::unordered_map<std::string, FloorData> m_floors;
};
