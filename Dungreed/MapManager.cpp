#include "MapManager.h"

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Vertex.hpp>
#include <SFML/Graphics/VertexArray.hpp>

#include <algorithm>
#include <functional>
#include <stdexcept>

MapManager& MapManager::getInstance() {
    static MapManager instance;
    return instance;
}

MapManager::MapManager() : m_random(std::random_device{}()) {}


void MapManager::setSeed(std::uint32_t seed) {
    m_random.seed(seed);
}


Room& MapManager::createRoom(RoomType type) {
    m_rooms.push_back(std::make_unique<Room>(type));
    return *m_rooms.back();
}

bool MapManager::owns(const Room& room) const {
    return std::any_of(m_rooms.begin(), m_rooms.end(), [&room](const std::unique_ptr<Room>& candidate) {
        return candidate.get() == &room;
    });
}

bool MapManager::linkRoom(Room& first, Room& second) {
    if (!owns(first) || !owns(second) || &first == &second || !first.canAddDoor() || !second.canAddDoor() ||
        first.isConnectedTo(second)) {
        return false;
    }

    // Both insertions are non-throwing in normal gameplay. Roll back if allocation fails.
    if (!first.addDoor(second)) {
        return false;
    }
    try {
        if (!second.addDoor(first)) {
            first.getInfo()->doors.pop_back();
            return false;
        }
    } catch (...) {
        first.getInfo()->doors.pop_back();
        throw;
    }
    return true;
}

bool MapManager::genRoom(std::size_t normalRoomCount, const MonsterSpawnConfig& spawnConfig) {
    if (normalRoomCount < 1) {
        return false;
    }

    std::vector<std::unique_ptr<Room>> previousRooms = std::move(m_rooms);
    Room* previousStart = m_start;
    Room* previousBoss = m_boss;
    m_rooms.clear();
    m_start = nullptr;
    m_boss = nullptr;

    try {
        m_start = &createRoom(RoomType::Start);
        std::vector<Room*> normalRooms;
        normalRooms.reserve(normalRoomCount);
        for (std::size_t index = 0; index < normalRoomCount; ++index) {
            normalRooms.push_back(&createRoom(RoomType::Normal));
        }
        Room& hut = createRoom(RoomType::Hut);
        m_boss = &createRoom(RoomType::Boss);

        if (!linkRoom(*m_start, *normalRooms.front())) {
            throw std::logic_error("Unable to link Town to the dungeon.");
        }
        for (std::size_t index = 1; index < normalRooms.size(); ++index) {
            if (!linkRoom(*normalRooms[index - 1], *normalRooms[index])) {
                throw std::logic_error("Unable to link normal rooms.");
            }
        }
        if (!linkRoom(*normalRooms.back(), *m_boss)) {
            throw std::logic_error("Unable to link Boss room.");
        }

        std::uniform_int_distribution<std::size_t> firstBonus(0, normalRooms.size() - 1);
        const std::size_t shopIndex = firstBonus(m_random);
        std::uniform_int_distribution<std::size_t> secondBonus(0, normalRooms.size() - 2);
        std::size_t hutIndex = secondBonus(m_random);
        if (hutIndex >= shopIndex) {
            ++hutIndex;
        }
        if (!linkRoom(*normalRooms[shopIndex], shop) || !linkRoom(*normalRooms[hutIndex], hut)) {
            throw std::logic_error("Unable to link bonus rooms.");
        }

        for (const std::unique_ptr<Room>& room : m_rooms) {
            room->genMonster(spawnConfig, m_random);
            room->genChest();
        }
    } catch (...) {
        m_rooms = std::move(previousRooms);
        m_start = previousStart;
        m_boss = previousBoss;
        return false;
    }

    return true;
}

sf::Color MapManager::minimapColor(RoomType type) const {
    switch (type) {
    case RoomType::Start: return sf::Color(80, 180, 255);
    case RoomType::Normal: return sf::Color(180, 180, 180);
    case RoomType::Shop: return sf::Color(255, 210, 80);
    case RoomType::Hut: return sf::Color(120, 210, 130);
    case RoomType::Boss: return sf::Color(235, 80, 80);
    }
    return sf::Color::White;
}

void MapManager::minimap(sf::RenderTarget& target, sf::Vector2f origin) const {
    constexpr float roomSize = 14.f;
    constexpr float stepX = 28.f;
    constexpr float mainY = 18.f;
    constexpr float bonusY = 46.f;

    const auto roomPosition = [this, origin, stepX, mainY, bonusY](const Room& room) {
        const auto& info = *room.getInfo();
        const auto normalBegin = m_rooms.begin() + (m_rooms.empty() ? 0 : 1);
        const auto normalEnd = std::find_if(normalBegin, m_rooms.end(), [](const std::unique_ptr<Room>& candidate) {
            return candidate->getInfo()->type != RoomType::Normal;
        });
        const auto normalCount = static_cast<float>(std::distance(normalBegin, normalEnd));

        if (info.type == RoomType::Town) return origin + sf::Vector2f(0.f, mainY);
        if (info.type == RoomType::Boss) return origin + sf::Vector2f((normalCount + 1.f) * stepX, mainY);

        if (info.type == RoomType::Normal) {
            const auto normal = std::find_if(normalBegin, normalEnd, [&room](const std::unique_ptr<Room>& candidate) {
                return candidate.get() == &room;
            });
            return origin + sf::Vector2f((static_cast<float>(std::distance(normalBegin, normal)) + 1.f) * stepX, mainY);
        }

        const Room* connection = info.doors.empty() ? nullptr : info.doors.front().next;
        sf::Vector2f position = origin;
        if (connection) {
            const auto connectedNormal = std::find_if(normalBegin, normalEnd, [connection](const std::unique_ptr<Room>& candidate) {
                return candidate.get() == connection;
            });
            if (connectedNormal != normalEnd) {
                position += sf::Vector2f((static_cast<float>(std::distance(normalBegin, connectedNormal)) + 1.f) * stepX, mainY);
            }
        }
        position.y += info.type == RoomType::Shop ? bonusY : -bonusY;
        return position;
    };

    for (const std::unique_ptr<Room>& room : m_rooms) {
        const sf::Vector2f from = roomPosition(*room) + sf::Vector2f(roomSize / 2.f, roomSize / 2.f);
        for (const Door& door : room->getInfo()->doors) {
            if (door.next && std::less<const Room*>{}(room.get(), door.next)) {
                sf::VertexArray line(sf::PrimitiveType::Lines, 2);
                line[0].position = from;
                line[0].color = sf::Color(120, 120, 120);
                line[1].position = roomPosition(*door.next) + sf::Vector2f(roomSize / 2.f, roomSize / 2.f);
                line[1].color = sf::Color(120, 120, 120);
                target.draw(line);
            }
        }
    }

    for (const std::unique_ptr<Room>& room : m_rooms) {
        const auto& info = *room->getInfo();
        const sf::Vector2f position = roomPosition(*room);

        sf::RectangleShape marker({ roomSize, roomSize });
        marker.setPosition(position);
        marker.setFillColor(minimapColor(info.type));
        target.draw(marker);
    }
}
