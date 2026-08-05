#include "Room.h"
#include <random>
#include <iostream>

Room::Room(RoomType type) {
    m_info = std::make_unique<RoomInfo>();
    m_info->type = type;
    m_info->isClear = (type == RoomType::Town || type == RoomType::Treasure); // 마을과 보물방은 기본 클리어 상태
    m_info->isVisited = false;
}

void Room::genMonster(const MonsterSpawnConfig& config) {
    if (m_info->type == RoomType::Town || m_info->type == RoomType::Treasure) {
        return; // 마을이나 보물방은 몬스터를 생성하지 않음
    }

    if (config.monsterRules.empty()) {
        return;
    }

    std::random_device rd;
    std::mt19937 gen(rd());

    sf::FloatRect bounds = m_info->tileMapVertices.getBounds();
    float roomArea = bounds.size.x * bounds.size.y;

    // 방 면적이 0이거나 초기화되지 않은 경우 기본값 설정
    if (roomArea <= 0.f) {
        roomArea = 10000.f;
    }

    // 방 크기에 비례하는 목표 총 몬스터 수 계산 (예: 면적 100당 약 3~5마리 비율)
    int calculatedTargetCount = static_cast<int>(roomArea / 5000.f);
    int targetTotalCount = std::clamp(calculatedTargetCount, config.minTotalMonsters, config.maxTotalMonsters);

    std::cout << "[Room] 몬스터 생성 시작 (방 면적 기반 목표 총 수량: " << targetTotalCount << "마리)" << std::endl;
    
    for (const auto& rule : config.monsterRules) {
        if (rule.minCount > rule.maxCount) continue;

        std::uniform_int_distribution<int> dist(rule.minCount, rule.maxCount);
        int countForThisType = dist(gen);

        for (int i = 0; i < countForThisType; ++i) {
            // TODO: Monster 객체를 생성하여 방 내부 리스트에 추가
            std::cout << " - 생성된 몬스터 종류: " << rule.monsterType << std::endl;
        }
    }
}
void Room::genChest() {//임시 구현
    if (m_info->type == RoomType::Treasure) {
        m_info->chest.gold = 100;
        m_info->chest.isOpened = false;
        std::cout << "[Room] 보물방 상자 생성 완료 (골드: 100)" << std::endl;
    }
}

void Room::update() {
    if (!m_info->isClear) {
        // 모든 몬스터 처치 시 isClear = true 처리 로직 추가 예정
    }
}