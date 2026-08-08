#pragma once

#include "Monster.h"
#include <memory>
#include <string>
#include <unordered_map>

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
    Actor::Status status{ MAXHP, MAXHP, POWER, DEX };
    MonsterBehaviorConfig behavior;
    std::string weaponId;
};

class GameDataManager {
public:
    bool loadWeapons(const std::string& path);
    bool loadMonsters(const std::string& path);
    const WeaponData* findWeapon(const std::string& id) const;
    const MonsterData* findMonster(const std::string& id) const;
    std::shared_ptr<Equip> createEquip(const std::string& weaponId) const;

private:
    std::unordered_map<std::string, WeaponData> m_weapons;
    std::unordered_map<std::string, MonsterData> m_monsters;
};