#include "MapManager.h"
#include<iostream>
#include <algorithm>
#include <functional>
#include <stdexcept>
#include <queue>

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


Room* MapManager::pickRandomConnectableRoom(const std::vector<Room*>& rooms, const Room* exclude) {
    std::vector<Room*> candidates;
    candidates.reserve(rooms.size());

    for (Room* room : rooms) {
        if (!room->canAddDoor())
            continue;

        if (exclude) {
            if (room == exclude)
                continue;

            if (exclude->isConnectedTo(*room))
                continue;
        }

        candidates.push_back(room);
    }

    if (candidates.empty()) {
        return nullptr;
    }

    std::uniform_int_distribution<std::size_t> pick(
        0,
        candidates.size() - 1);

    return candidates[pick(m_random)];
}


BFSResult MapManager::bfs(Room* start) const {

    BFSResult result;

    if (!start)
        return result;

    std::queue<Room*> queue;
    queue.push(start);

    result.distance[start] = 0;
    if (start->canAddDoor()) {
        result.farthestRoom = start;
    }

    while (!queue.empty()) {
        Room* current = queue.front();

        queue.pop();

        const std::size_t currentDistance = result.distance[current];
        //방을 추가 가능하면서
        if (current->canAddDoor()) {
            //현재 가장 먼 방으로 지정된게 아니면서 거리가 더 멀경우에만
            if (!result.farthestRoom ||
                currentDistance > result.distance[result.farthestRoom]) {
                result.farthestRoom = current;
            }
        }

        for (const Door& door : current->getInfo()->doors) {
            Room* next = door.next;
            //다음방이 없음
            if (!next)
                continue;
            //이미 체크한 방
            if (result.distance.find(next) != result.distance.end()) {
                continue;
            }

            //다음 방까지의 거리 계산
            result.distance[next] = currentDistance + 1;
            //해당 방에 연결된 방도 조사하기위해 큐에 삽입
            queue.push(next);
        }
    }

    return result;
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

        std::vector<Room*> normalRooms;
        normalRooms.reserve(normalRoomCount);
        for (std::size_t index = 0; index < normalRoomCount; ++index) {
            normalRooms.push_back(&createRoom(RoomType::Normal));
        }
        Room& hut = createRoom(RoomType::Hut);


        //연결된 방
        std::vector<Room*> connected;
        connected.reserve(normalRooms.size());
        connected.push_back(normalRooms.front());

        //연결 안된 방
        std::vector<Room*> unconnected;
        unconnected.reserve(normalRooms.size() - 1);
        for (std::size_t i = 1; i < normalRooms.size(); ++i) {
            unconnected.push_back(normalRooms[i]);
        }
        //연결 안된 방 벡터를 순회하면서 연결에 추가
        while (!unconnected.empty()) {
            //연결된 방 중 랜덤 방 선택하여 부모방으로 지정
            Room* parent = pickRandomConnectableRoom(connected);
            std::uniform_int_distribution<std::size_t> pick(0, unconnected.size() - 1);
            Room* child = unconnected[pick(m_random)];

            if (!parent || !child) {
                throw std::logic_error("No connectable rooms.");
            }
            //두 방이 이미 연결상태면 스킵
            if (!linkRoom(*parent, *child))
                continue;

            //연결 완료 후 자식 방도 이제 연결된 방 목록에 포함
            connected.push_back(child);

            //연결 안된 방 리스트 갱신
            unconnected.erase(
                std::remove(unconnected.begin(), unconnected.end(), child),
                unconnected.end());
        }
        //랜덤 간선 추가


        //추가 간선의 갯수는 전체 방 갯수의 절반
        const int extraEdges = static_cast<int>(normalRoomCount / 2);
        int created = 0;
        int retry = 0;

        while (created < extraEdges && retry < extraEdges * 10) {
            ++retry;


            Room* a = pickRandomConnectableRoom(normalRooms);
            if (!a)
                break;

            Room* b = pickRandomConnectableRoom(normalRooms, a);
            if (!b)
                continue;

            if (!linkRoom(*a, *b))
                continue;

            ++created;
        }

        //서로 가장 먼 방 탐색
        //시간 나면 해당 알고리즘 재확인
        auto first = bfs(normalRooms.front());

        if (!first.farthestRoom) {
            throw std::logic_error("Unable to find start room.");
        }

        auto second = bfs(first.farthestRoom);

        if (!second.farthestRoom) {
            throw std::logic_error("Unable to find boss room.");
        }

        Room* startRoom = first.farthestRoom;
        Room* bossRoom = second.farthestRoom;

        Room& start = createRoom(RoomType::Start);
        Room& boss = createRoom(RoomType::Boss);

        m_start = &start;
        m_boss = &boss;
        if (!linkRoom(*m_start, *startRoom)) {
            throw std::logic_error("Unable to link Start room.");
        }

        if (!linkRoom(*bossRoom, *m_boss)) {
            throw std::logic_error("Unable to link Boss room.");
        }
        
        Room* hutRoom = pickRandomConnectableRoom(normalRooms);

        if (!hutRoom) {
            throw std::logic_error("Unable to find Hut room.");
        }

        if (!linkRoom(*hutRoom, hut)) {
            throw std::logic_error("Unable to link Hut room.");
        }
        for (const std::unique_ptr<Room>& room : m_rooms) {
            room->genMonster(spawnConfig, m_random);
            room->genChest();
        }
    } catch (...) {
        std::cerr << "문제 발생. 맵 생성 오류, 기존의 맵을 재사용합니다\n";
        m_rooms = std::move(previousRooms);
        m_start = previousStart;
        m_boss = previousBoss;
        return false;
    }

    return true;
}


