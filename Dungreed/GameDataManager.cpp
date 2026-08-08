#include "GameDataManager.h"

#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace {
WeaponType parseWeaponType(const std::string& value) {
    return value == "Ranged" ? WeaponType::Ranged : WeaponType::Melee;
}

ProjectileType parseProjectileType(const std::string& value) {
    if (value == "Fireball") return ProjectileType::Fireball;
    if (value == "Bullet") return ProjectileType::Bullet;
    if (value == "BabyBatBullet") return ProjectileType::BabyBatBullet;
    return ProjectileType::Arrow;
}

ProjectileTarget parseTarget(const std::string& value) {
    return value == "Player" ? ProjectileTarget::Player : ProjectileTarget::Monster;
}
}

bool GameDataManager::loadWeapons(const std::string& path) {
    std::ifstream file(path);
    if (!file) return false;
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
    if (!file) return false;
    json root;
    file >> root;
    m_monsters.clear();

    for (const auto& entry : root.at("monsters")) {
        MonsterData data;
        data.id = entry.at("id").get<std::string>();
        data.enabled = entry.value("enabled", true);
        data.atlasKey = entry.value("atlasKey", "Monster");
        const auto& status = entry.at("status");
        data.status.maxHp = status.value("maxHp", MAXHP);
        data.status.tmpHp = data.status.maxHp;
        data.status.power = status.value("power", POWER);
        data.status.dex = status.value("dex", DEX);
        const auto& ai = entry.at("ai");
        data.behavior.detectRange = ai.value("detectRange", 100.f);
        data.behavior.attackRange = ai.value("attackRange", 50.f);
        if (entry.contains("movement")) {
            data.behavior.moveSpeed = entry.at("movement").value("moveSpeed", 300.f);
        }
        data.weaponId = entry.value("weaponId", "");
        m_monsters.emplace(data.id, std::move(data));
    }
    return true;
}

const WeaponData* GameDataManager::findWeapon(const std::string& id) const {
    const auto it = m_weapons.find(id);
    return it == m_weapons.end() ? nullptr : &it->second;
}

const MonsterData* GameDataManager::findMonster(const std::string& id) const {
    const auto it = m_monsters.find(id);
    return it == m_monsters.end() ? nullptr : &it->second;
}

std::shared_ptr<Equip> GameDataManager::createEquip(const std::string& weaponId) const {
    const WeaponData* data = findWeapon(weaponId);
    if (!data) return nullptr;
    auto equip = std::make_shared<Equip>(data->id, data->stat);
    if (!data->atlasKey.empty() && !data->frame.empty()) {
        equip->init(data->atlasKey, data->frame);
    }
    return equip;
}