#include "DebugManager.h"

#include <algorithm>
#include <iostream>

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>

#include "Monster.h"
#include "GameDataManager.h"
#include "LogManager.h"
#include "MapManager.h"
#include "ObjectPoolingManager.h"
#include "Player.h"
#include "Projectile.h"
#include "Room.h"
#include "TileMap.h"

namespace {
void drawBounds(sf::RenderWindow &window, const sf::FloatRect &bounds, const sf::Color &outlineColor, float outlineThickness = 1.5f) {
    sf::RectangleShape shape(bounds.size);
    shape.setPosition(bounds.position);
    shape.setFillColor(sf::Color(0, 0, 0, 0));
    shape.setOutlineColor(outlineColor);
    shape.setOutlineThickness(outlineThickness);
    window.draw(shape);
}

void drawCircularRange(sf::RenderWindow &window, const sf::Vector2f &center, float range, const sf::Color &fillColor, const sf::Color &outlineColor) {
    if (range <= 0.f) {
        return;
    }

    sf::CircleShape shape(range);
    shape.setOrigin({range, range});
    shape.setPosition(center);
    shape.setFillColor(fillColor);
    shape.setOutlineColor(outlineColor);
    shape.setOutlineThickness(1.5f);
    window.draw(shape);
}
} // namespace

DebugManager::~DebugManager() = default;

DebugCommand DebugManager::readConsoleCommand(const GameDataManager &gameData) const {
    while (true) {
        std::cout << "\n========== DEBUG MENU ==========\n"
                  << "1. Spawn room\n"
                  << "2. Toggle combat bounds\n"
                  << "0. Cancel\n"
                  << "Select: " << std::flush;

        std::string selection;
        if (!std::getline(std::cin, selection)) {
            std::cin.clear();
            return {};
        }

        if (selection == "0") {
            return {};
        }
        if (selection == "2") {
            return {DebugCommandType::ToggleCombatBounds};
        }
        if (selection == "1") {
            while (true) {
                DebugCommand command;
                command.type = DebugCommandType::SpawnRoom;
                std::cout << "Floor ID: " << std::flush;
                if (!std::getline(std::cin, command.floorId)) {
                    std::cin.clear();
                    return {};
                }
                std::cout << "Room ID: " << std::flush;
                if (!std::getline(std::cin, command.roomId)) {
                    std::cin.clear();
                    return {};
                }

                const FloorData *floor = gameData.findFloor(command.floorId);
                if (!floor) {
                    std::cout << "Unknown Floor ID. Try again.\n";
                    continue;
                }
                if (floor->rooms.find(command.roomId) == floor->rooms.end()) {
                    std::cout << "Unknown Room ID for this floor. Try again.\n";
                    continue;
                }
                return command;
            }
        }

        std::cout << "Invalid selection.\n";
    }
}

bool DebugManager::spawnRoom(const std::string &floorId, const std::string &roomId, const GameDataManager &gameData, MapManager &mapManager, const std::string &tileAtlasKey, const RoomTileSet &tileSet) const {
    const FloorData *floor = gameData.findFloor(floorId);
    if (!floor || floor->rooms.find(roomId) == floor->rooms.end()) {
        LogManager::getInstance().warning("DebugManager", "디버그 생성 대상 층 또는 방을 찾을 수 없습니다: " + floorId + '/' + roomId);
        return false;
    }

    return mapManager.createCurrentRoomFromData(*floor, roomId) && mapManager.preloadFloorTileMaps(tileAtlasKey, tileSet);
}

void DebugManager::renderCombatBounds(sf::RenderWindow &window, const Player &player, ObjectPoolingManager &objectPool) const {
    // 초록색: 플레이어가 피해를 받는 실제 몸체 충돌 상자입니다.
    drawBounds(window, player.getCollision().getHitbox(), sf::Color::Green);
    if (const auto weapon = player.getEquipment(); weapon && weapon->isAttacking()) {
        if (const auto attackBounds = player.getAttackHitbox()) {
            // 빨간색: 이번 스윙 중 몬스터에게 피해를 줄 수 있는 무기 충돌 상자입니다.
            drawBounds(window, *attackBounds, sf::Color::Red, 2.f);
        }
    }

    objectPool.forEachActiveMonster([&](Monster &monster) {
        if (monster.dead()) {
            return;
        }

        // 청록색: 몬스터가 피해를 받는 실제 몸체 충돌 상자입니다.
        drawBounds(window, monster.getCollision().getHitbox(), sf::Color::Cyan);
        const bool isAttackActive = monster.isAttackDamageWindowActive();
        const auto weapon = monster.getEquipment();
        const bool isMelee = weapon && weapon->getStat().type == WeaponType::Melee;
        const sf::Vector2f center = monster.getBodyCenterPosition();

        // 파란 원: 플레이어를 감지해 추적을 시작하는 범위입니다.
        drawCircularRange(window, center, monster.getDetectRange(), sf::Color(70, 160, 255, 10), sf::Color(70, 160, 255, 100));
        // 노란 원: 공격 상태 진입 여부를 판단하는 설정 사거리입니다.
        drawCircularRange(window, center, monster.getAttackRange(), sf::Color(255, 190, 60, 12), sf::Color(255, 190, 60, 150));

        if (isMelee) {
            // 빨간 상자: 근접 공격이 실제로 유효한 몬스터 전면 충돌 영역입니다.
            drawBounds(window, monster.getFrontAttackBounds(), isAttackActive ? sf::Color(255, 70, 70, 230) : sf::Color(255, 70, 70, 90), isAttackActive ? 2.f : 1.5f);
        }
        if (monster.getAttackPattern() == MonsterAttackPattern::ChargeCombo && monster.state == MonsterState::Charge) {
            // 돌진 공격의 실제 판정은 몸체 충돌이므로 주황색 상자로 표시합니다.
            drawBounds(window, monster.getCollision().getHitbox(), sf::Color(255, 160, 0), 2.f);
        }
    });

    objectPool.forEachActiveProjectile([&](Projectile &projectile) {
        if (projectile.getTarget() == ProjectileTarget::Player && projectile.isDamageActive()) {
            // 빨간 상자: 원거리 몬스터 투사체가 실제 피해를 주는 충돌 영역입니다.
            drawBounds(window, projectile.getGlobalBounds(), sf::Color::Red, 2.f);
        }
    });
}

bool DebugManager::buildRoomPreviews(const std::vector<const Room *> &rooms, const std::string &tileAtlasKey, const RoomTileSet &tileSet, sf::Vector2u viewportSize) {
    constexpr float maxPreviewScale = 0.45f;
    constexpr float margin = 28.f;

    m_roomPreviews.clear();
    if (rooms.empty()) {
        LogManager::getInstance().warning("DebugManager", "미리 보기로 만들 방 목록이 비어 있습니다.");
        return false;
    }

    for (const Room *room : rooms) {
        if (!room) {
            LogManager::getInstance().warning("DebugManager", "방 미리 보기 목록에 null 방이 포함되어 있습니다.");
            m_roomPreviews.clear();
            return false;
        }

        auto preview = std::make_unique<TileMap>();
        if (!room->buildTileMap(*preview, tileAtlasKey, tileSet)) {
            LogManager::getInstance().warning("DebugManager", "방 미리 보기용 타일맵 생성에 실패했습니다.");
            m_roomPreviews.clear();
            return false;
        }
        m_roomPreviews.push_back(std::move(preview));
    }

    // 가능한 열 개수를 비교해, 모든 프리뷰가 화면에 들어가는 가장 큰 배율을 선택합니다.
    const std::size_t roomCount = m_roomPreviews.size();
    std::size_t bestColumnCount = 1;
    float bestScale = 0.f;
    for (std::size_t columnCount = 1; columnCount <= roomCount; ++columnCount) {
        const std::size_t rowCount = (roomCount + columnCount - 1) / columnCount;
        std::vector<float> columnWidths(columnCount, 0.f);
        std::vector<float> rowHeights(rowCount, 0.f);
        for (std::size_t index = 0; index < roomCount; ++index) {
            const sf::Vector2f size = m_roomPreviews[index]->getPixelSize();
            const std::size_t column = index % columnCount;
            const std::size_t row = index / columnCount;
            columnWidths[column] = std::max(columnWidths[column], size.x);
            rowHeights[row] = std::max(rowHeights[row], size.y);
        }

        float totalWidth = 0.f;
        for (const float width : columnWidths) {
            totalWidth += width;
        }
        float totalHeight = 0.f;
        for (const float height : rowHeights) {
            totalHeight += height;
        }
        const float availableWidth = std::max(1.f, static_cast<float>(viewportSize.x) - margin * (columnCount + 1));
        const float availableHeight = std::max(1.f, static_cast<float>(viewportSize.y) - margin * (rowCount + 1));
        const float scale = std::min({maxPreviewScale, availableWidth / totalWidth, availableHeight / totalHeight});
        if (scale > bestScale) {
            bestScale = scale;
            bestColumnCount = columnCount;
        }
    }

    const std::size_t rowCount = (roomCount + bestColumnCount - 1) / bestColumnCount;
    std::vector<float> columnWidths(bestColumnCount, 0.f);
    std::vector<float> rowHeights(rowCount, 0.f);
    for (std::size_t index = 0; index < roomCount; ++index) {
        const sf::Vector2f size = m_roomPreviews[index]->getPixelSize();
        const std::size_t column = index % bestColumnCount;
        const std::size_t row = index / bestColumnCount;
        columnWidths[column] = std::max(columnWidths[column], size.x);
        rowHeights[row] = std::max(rowHeights[row], size.y);
    }

    std::vector<float> columnPositions(bestColumnCount, margin);
    std::vector<float> rowPositions(rowCount, margin);
    for (std::size_t column = 1; column < bestColumnCount; ++column) {
        columnPositions[column] = columnPositions[column - 1] + columnWidths[column - 1] * bestScale + margin;
    }
    for (std::size_t row = 1; row < rowCount; ++row) {
        rowPositions[row] = rowPositions[row - 1] + rowHeights[row - 1] * bestScale + margin;
    }
    for (std::size_t index = 0; index < roomCount; ++index) {
        const std::size_t column = index % bestColumnCount;
        const std::size_t row = index / bestColumnCount;
        TileMap &preview = *m_roomPreviews[index];
        preview.setPosition({columnPositions[column], rowPositions[row]});
        preview.setScale({bestScale, bestScale});
    }
    return true;
}

void DebugManager::renderRoomPreviews(sf::RenderWindow &window) const {
    for (const auto &preview : m_roomPreviews) {
        window.draw(*preview);
    }
}
