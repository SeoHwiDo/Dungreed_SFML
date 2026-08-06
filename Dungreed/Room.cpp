#include "Room.h"
#include "MonsterManager.h"
#include <algorithm>
#include <iostream>
#include <numeric>

namespace {
bool isNonCombatRoom(RoomType type) {
    return type == RoomType::Start || type == RoomType::Shop || type == RoomType::Hut;
}
}

Room::Room(RoomType type) : m_info(std::make_unique<RoomInfo>()) {
    m_info->type = type;
    m_info->isClear = isNonCombatRoom(type);
}

std::size_t Room::doorCount() const {
    return m_info->doors.size();
}

bool Room::canAddDoor() const {
    switch (m_info->type) {
    case RoomType::Normal:
        return doorCount() < 3;
    case RoomType::Start:
    case RoomType::Shop:
    case RoomType::Hut:
    case RoomType::Boss:
        return doorCount() == 0;
    }
    return false;
}

bool Room::isConnectedTo(const Room& room) const {
    return std::any_of(m_info->doors.begin(), m_info->doors.end(), [&room](const Door& door) {
        return door.next == &room;
    });
}

bool Room::addDoor(Room& room) {
    if (&room == this || !canAddDoor() || isConnectedTo(room)) {
        return false;
    }
    m_info->doors.push_back({ &room, true });
    return true;
}

void Room::genChest() {
    if (m_info->type == RoomType::Hut) {
        m_info->chest.gold = 100;
        m_info->chest.isOpened = false;
    }
}

void Room::update() {
    if (!m_info->isClear && MonsterManager::getInstance().getAliveMonsterCount() == 0) {
        m_info->isClear = true;
    }
}
