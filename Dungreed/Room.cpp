#include "Room.h"

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

void Room::genMonster(const MonsterSpawnConfig& config, std::mt19937& random) {
    if (isNonCombatRoom(m_info->type) || config.monsterRules.empty()) {
        m_monsters.clear();
        m_info->spawnedMonsterTypes.clear();
        return;
    }

    if (config.TotalMonsters < 0 || config.monstersPerSquareUnit < 0.f) {
        std::cerr << "[Room] 몬스터 생성 설정이 유효하지 않습니다.\n";
        return;
    }

    int minimumByType = 0;
    for (const MonsterRule& rule : config.monsterRules) {
        if (rule.monsterType.empty() || rule.minCount < 0 || rule.maxCount < rule.minCount) {
            std::cerr << "[Room] 몬스터 생성 규칙이 유효하지 않습니다.\n";
            return;
        }
        minimumByType += rule.minCount;
    }

    const sf::FloatRect bounds = m_info->tileMapVertices.getBounds();
    const float area = std::max(0.f, bounds.size.x) * std::max(0.f, bounds.size.y);
    const int areaMonsterCount = static_cast<int>(area * config.monstersPerSquareUnit);
    const int spawnableCount = config.TotalMonsters + areaMonsterCount;

    std::vector<int> counts;
    counts.reserve(config.monsterRules.size());
    for (const MonsterRule& rule : config.monsterRules) {
        //몬스터 별 최소 소환 수 배열
        counts.push_back(rule.minCount);
    }

    if (spawnableCount < minimumByType) {
        std::cerr << "[Room] 소환 가능 몬스터 수(" << spawnableCount
                  << ")가 종류별 최소 소환 수 합(" << minimumByType
                  << ")보다 작습니다. 가능한 최소 수만 소환합니다.\n";

        int remaining = spawnableCount;
        for (std::size_t index = 0; index < counts.size() && remaining > 0; ++index) {
            //현재 전체 소환 가능수와 해당 몬스터의 최소 소환수 비교하여 더 적은 수만큼 생성
            counts[index] = std::min(counts[index], remaining);
            //남은 몬스터 계산
            remaining -= counts[index];
        }
    } else {
            //각 몬스터 별 최대 소환 수 배열
        for (int remaining = spawnableCount - minimumByType; remaining > 0; --remaining) {
            //남은 전체 수를 채우기 위해 선택할 몬스터의 랜덤풀
            std::vector<std::size_t> candidates;
            for (std::size_t index = 0; index < config.monsterRules.size(); ++index) {
                //아직 해당 몬스터의 상한선에 도달하지 않은 몬스터만 풀에 추가
                if (counts[index] < config.monsterRules[index].maxCount) {
                    candidates.push_back(index);
                }
            }
            if (candidates.empty()) {
                break;
            }
            //현재 풀에서 몬스터 랜덤 선택 후 추가
            std::uniform_int_distribution<std::size_t> pick(0, candidates.size() - 1);
            ++counts[candidates[pick(random)]];
        }
    }
    //정보 배열 초기화
    m_monsters.clear();
    m_info->spawnedMonsterTypes.clear();
    //전체 소환한 몬스터 수 계산
    const int spawnedCount = std::accumulate(counts.begin(), counts.end(), 0);

    //미리 메모리 준비
    m_monsters.reserve(static_cast<std::size_t>(spawnedCount));
    m_info->spawnedMonsterTypes.reserve(static_cast<std::size_t>(spawnedCount));

    //규칙대로 몬스터 소환 및 몬스터 종류 리스트 생성
    for (std::size_t index = 0; index < config.monsterRules.size(); ++index) {
        for (int count = 0; count < counts[index]; ++count) {
            m_monsters.push_back(std::make_unique<Monster>(config.monsterRules[index].monsterType));
            m_info->spawnedMonsterTypes.push_back(config.monsterRules[index].monsterType);
        }
    }
}

void Room::genChest() {
    if (m_info->type == RoomType::Hut) {
        m_info->chest.gold = 100;
        m_info->chest.isOpened = false;
    }
}

void Room::update() {
    if (!m_info->isClear && m_monsters.empty()) {
        m_info->isClear = true;
    }
}
