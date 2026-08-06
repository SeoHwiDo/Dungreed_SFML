#include "MonsterManager.h"
#include <iostream>
#include <numeric>
#include <algorithm>

namespace {
    bool isNonCombatRoom(RoomType type) {
        return type == RoomType::Start || type == RoomType::Shop || type == RoomType::Hut;
    }
}
void MonsterManager::spawnMonsters(Room* room, const MonsterSpawnConfig& config, std::mt19937& random) {
    if (!room || !room->getInfo()) return;

    RoomInfo* info = room->getInfo();

    // 전투 방이 아니거나 스폰 규칙이 없으면 생략
    if (isNonCombatRoom(info->type) || config.monsterRules.empty()) {
        info->spawnedMonsterTypes.clear();
        return;
    }

    if (config.TotalMonsters < 0 || config.monstersPerSquareUnit < 0.f) {
        std::cerr << "[MonsterManager] 몬스터 생성 설정이 유효하지 않습니다.\n";
        return;
    }

    int minimumByType = 0;
    for (const MonsterRule& rule : config.monsterRules) {
        if (rule.monsterType.empty() || rule.minCount < 0 || rule.maxCount < rule.minCount) {
            std::cerr << "[MonsterManager] 몬스터 생성 규칙이 유효하지 않습니다.\n";
            return;
        }
        minimumByType += rule.minCount;
    }

    const sf::FloatRect bounds = info->tileMapVertices.getBounds();
    const float area = std::max(0.f, bounds.size.x) * std::max(0.f, bounds.size.y);
    const int areaMonsterCount = static_cast<int>(area * config.monstersPerSquareUnit);
    const int spawnableCount = config.TotalMonsters + areaMonsterCount;

    std::vector<int> counts;
    counts.reserve(config.monsterRules.size());
    for (const MonsterRule& rule : config.monsterRules) {
        counts.push_back(rule.minCount); // 몬스터 별 최소 소환 수
    }

    if (spawnableCount < minimumByType) {
        std::cerr << "[MonsterManager] 소환 가능 수(" << spawnableCount
            << ")가 최소 소환 수 합보다 작습니다. 최소 수만 소환합니다.\n";

        int remaining = spawnableCount;
        for (std::size_t index = 0; index < counts.size() && remaining > 0; ++index) {
            counts[index] = std::min(counts[index], remaining);
            remaining -= counts[index];
        }
    } else {
        for (int remaining = spawnableCount - minimumByType; remaining > 0; --remaining) {
            std::vector<std::size_t> candidates;
            for (std::size_t index = 0; index < config.monsterRules.size(); ++index) {
                if (counts[index] < config.monsterRules[index].maxCount) {
                    candidates.push_back(index);
                }
            }
            if (candidates.empty()) break;

            std::uniform_int_distribution<std::size_t> pick(0, candidates.size() - 1);
            ++counts[candidates[pick(random)]];
        }
    }

    info->spawnedMonsterTypes.clear();
    const int spawnedCount = std::accumulate(counts.begin(), counts.end(), 0);

    // 기존 몬스터 배열에 추가로 이어서 할당 (메모리 재할당 방지)
    m_monsters.reserve(m_monsters.size() + static_cast<std::size_t>(spawnedCount));
    info->spawnedMonsterTypes.reserve(static_cast<std::size_t>(spawnedCount));

    // 규칙대로 몬스터 소환 및 매니저에 소유권 등록
    for (std::size_t index = 0; index < config.monsterRules.size(); ++index) {
        for (int count = 0; count < counts[index]; ++count) {
            auto newMonster = std::make_unique<Monster>(config.monsterRules[index].monsterType);

            // TODO: 생성된 몬스터의 위치를 방 크기(bounds) 내 무작위로 분산시키는 로직 추가 가능

            m_monsters.push_back(std::move(newMonster));
            info->spawnedMonsterTypes.push_back(config.monsterRules[index].monsterType);
        }
    }
}

void MonsterManager::clearAll() {
    m_monsters.clear();
}

void MonsterManager::setTargetPosition(const sf::Vector2f& targetPos) {
    m_targetPos = targetPos;
    m_hasTarget = true;
}

void MonsterManager::clearTarget() {
    m_hasTarget = false;
}

void MonsterManager::updateAll(float dt) {
    if (m_monsters.empty()) return;

    // ========================================================================
    // [1] 싱글 스레드 방식
    // ========================================================================
    for (auto& monster : m_monsters) {
        if (m_hasTarget) {
            monster->setTargetPos(m_targetPos);
        } else {
            monster->clearTarget();
        }
        monster->update(dt);
    }

    // ========================================================================
    // [2] 멀티 스레드 방식 (추후 프레임레이트 비교 테스트용 예비 코드)
    // ========================================================================
    /*
    std::vector<std::thread> threads;
    threads.reserve(m_monsters.size());
    for (auto& monster : m_monsters) {
        threads.emplace_back([this, &monster, dt]() {
            if (m_hasTarget) monster->setTargetPos(m_targetPos);
            else monster->clearTarget();
            monster->update(dt);
        });
    }
    for (auto& t : threads) {
        if (t.joinable()) t.join();
    }
    */
}

size_t MonsterManager::getAliveMonsterCount() const {
    size_t count = 0;
    for (const auto& monster : m_monsters) {
        if (monster->state != MonsterState::Dead) {
            count++;
        }
    }
    return count;
}