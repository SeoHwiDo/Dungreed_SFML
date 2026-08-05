#pragma once
#include<vector>
#include<string>
#include<SFML/Graphics.hpp>
enum class RoomType {
    Town,       // 마을 (던전 입구 존재)
    Normal,     // 일반 전투 방
    Treasure,   // 보물 방(추후 구현 확장용)
    Boss        // 보스 방
};

// 전방 선언 (순환 참조 방지)
class Room;

// 문(Door) 구조체: 방과 방을 연결
struct door {
    Room* prev = nullptr;
    Room* next = nullptr;
    bool isOpen = true;
};

// 보물상자 및 아이템 데이터 구조체 (추후 구현 확장용)
struct ChestData {
    int gold = 0;
    bool isOpened = false;
};

struct MonsterRule {
    std::string monsterType; // 몬스터 종류 이름
    int minCount = 0;        // 해당 종류의 최소 생성 수
    int maxCount = 0;        // 해당 종류의 최대 생성 수
};

// 몬스터 전체 스폰 설정을 위한 구조체
struct MonsterSpawnConfig {
    std::vector<MonsterRule> monsterRules; // 몬스터 종류별 최소/최대 규칙 리스트
    int minTotalMonsters = 2;              // 방별 최소 총 몬스터 수
    int maxTotalMonsters = 8;              // 방별 최대 총 몬스터 수
};

struct RoomInfo {
    std::vector<door> doors;           // 방에 연결된 문 목록
    RoomType type = RoomType::Normal;  // 방의 타입
    ChestData chest;                   // 보물상자 정보
    bool isClear = false;              // 방 클리어 여부
    bool isVisited = false;
    // [추가] 타일맵 렌더링을 위한 VertexArray 및 지형 데이터
    sf::VertexArray tileMapVertices;   // 방 내부 구조물을 그릴 VertexArray
    std::string tileAtlasKey = "";     // 사용할 타일셋 아틀라스 키 (예: "TileMap")
};
class Room{
private:
    std::unique_ptr<RoomInfo> m_info;

public:
    Room(RoomType type);
    ~Room() = default;

    // 방 정보 가져오기
    inline RoomInfo* getInfo() const { return m_info.get(); }

    // 몬스터 생성
    void genMonster(const MonsterSpawnConfig& config);

    // 보물상자 생성
    void genChest();

    // 방 클리어 상태 체크 및 업데이트
    void update();
};

