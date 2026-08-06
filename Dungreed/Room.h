#pragma once

#include "Monster.h"

#include <SFML/Graphics.hpp>

#include <cstddef>
#include <memory>
#include <random>
#include <string>
#include <vector>

enum class RoomType {
    Start,
    Town, // Compatibility name used by MapManager and the class diagram.
    Normal,
    Hut,
    Shop,
    Boss
};

class Room;

// A door only needs to observe the room at the other end.
struct Door {
    Room* next = nullptr;
    bool isOpen = true;
};

struct ChestData {
    int gold = 0;
    bool isOpened = false;
};

struct MonsterRule {
    std::string monsterType;
    int minCount = 0;
    int maxCount = 0;
};

struct MonsterSpawnConfig {
    std::vector<MonsterRule> monsterRules;
    int TotalMonsters = 8; // Base count, before area-based additions.
    float monstersPerSquareUnit = 1.f / 5000.f;
};

struct RoomInfo {
    std::vector<Door> doors;
    RoomType type = RoomType::Normal;
    ChestData chest;
    bool isClear = false;
    bool isVisited = false;
    sf::VertexArray tileMapVertices;
    std::string tileAtlasKey;
    // Lightweight spawn log for UI/debugging. Room owns the objects themselves.
    std::vector<std::string> spawnedMonsterTypes;
};

class Room {
public:
    explicit Room(RoomType type);
    ~Room() = default;

    Room(const Room&) = delete;
    Room& operator=(const Room&) = delete;

    RoomInfo* getInfo() { return m_info.get(); }
    const RoomInfo* getInfo() const { return m_info.get(); }

    std::size_t doorCount() const;
    bool canAddDoor() const;
    bool isConnectedTo(const Room& room) const;
    bool addDoor(Room& room);

    void genChest();
    void update();

private:
    std::unique_ptr<RoomInfo> m_info;
};
