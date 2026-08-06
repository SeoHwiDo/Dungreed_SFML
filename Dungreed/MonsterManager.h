#pragma once
#include <vector>
#include <memory>
#include <random>
#include <SFML/System/Vector2.hpp>
//-----------------------------------------------------
#include "Monster.h"
#include "Room.h" 
class MonsterManager {
public:
    //=============================== Singleton Pattern ==============================
    static MonsterManager& getInstance() {
        static MonsterManager instance;
        return instance;
    }
    MonsterManager(const MonsterManager&) = delete;
    MonsterManager& operator=(const MonsterManager&) = delete;
    //================================================================================

    void spawnMonsters(Room* room, const MonsterSpawnConfig& config, std::mt19937& random); 
    // Room에서 몬스터 생성/소멸 시 매니저에 등록 및 해제합니다.

    void clearAll(); // 방 이동 시 전체 초기화용

    // 매 프레임 플레이어 위치를 받아옵니다.
    void setTargetPosition(const sf::Vector2f& targetPos);
    void clearTarget();

    // 현재 방이 클리어되었는지 파악하기 위한 유틸
    size_t getAliveMonsterCount() const;
    // 등록된 전체 몬스터의 FSM 로직 및 물리 연산을 수행합니다.
    void updateAll(float dt);

private:
    MonsterManager() = default;
    ~MonsterManager() = default;

    std::vector<std::unique_ptr<Monster>> m_monsters;

    sf::Vector2f m_targetPos{ 0.f, 0.f };
    bool m_hasTarget = false;
};