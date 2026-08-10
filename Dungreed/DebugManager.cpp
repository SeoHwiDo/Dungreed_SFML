#include "DebugManager.h"

#include <algorithm>

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>

#include "Monster.h"
#include "ObjectPoolingManager.h"
#include "Player.h"
#include "Room.h"
#include "TileMap.h"

namespace {
void drawBounds(sf::RenderWindow& window, const sf::FloatRect& bounds,
    const sf::Color& outlineColor, float outlineThickness = 1.5f) {
    sf::RectangleShape shape(bounds.size);
    shape.setPosition(bounds.position);
    shape.setFillColor(sf::Color(0, 0, 0, 0));
    shape.setOutlineColor(outlineColor);
    shape.setOutlineThickness(outlineThickness);
    window.draw(shape);
}

void drawMonsterAttackRange(sf::RenderWindow& window, const Monster& monster,
    bool isAttackActive) {
    const float range = monster.getAttackRange();
    if (range <= 0.f) {
        return;
    }

    sf::CircleShape shape(range);
    shape.setOrigin({ range, range });
    shape.setPosition(monster.getBodyCenterPosition());
    shape.setFillColor(isAttackActive ? sf::Color(255, 80, 80, 24) :
        sf::Color(255, 190, 60, 16));
    shape.setOutlineColor(isAttackActive ? sf::Color(255, 80, 80, 220) :
        sf::Color(255, 190, 60, 130));
    shape.setOutlineThickness(1.5f);
    window.draw(shape);
}
}

DebugManager::~DebugManager() = default;

void DebugManager::renderCombatBounds(sf::RenderWindow& window,
    const Player& player, ObjectPoolingManager& objectPool) const {
    // 초록색: 플레이어가 피해를 받는 실제 몸체 충돌 상자입니다.
    drawBounds(window, player.getCollision().getHitbox(), sf::Color::Green);
    if (const auto weapon = player.getEquipment(); weapon && weapon->isAttacking()) {
        if (const auto attackBounds = player.getAttackHitbox()) {
            // 빨간색: 이번 스윙 중 몬스터에게 피해를 줄 수 있는 무기 충돌 상자입니다.
            drawBounds(window, *attackBounds, sf::Color::Red, 2.f);
        }
    }

    objectPool.forEachActiveMonster([&](Monster& monster) {
        if (monster.dead()) {
            return;
        }

        // 청록색: 몬스터가 피해를 받는 실제 몸체 충돌 상자입니다.
        drawBounds(window, monster.getCollision().getHitbox(), sf::Color::Cyan);
        const bool isAttackActive = monster.state == MonsterState::Attack;
        // 노란 원은 설정된 공격 사거리, 붉은 원은 현재 실제 공격 중인 사거리입니다.
        drawMonsterAttackRange(window, monster, isAttackActive);
        if (isAttackActive && monster.requiresBodyContactAttack()) {
            // 접촉 공격 몬스터는 사거리 외에도 몸체 겹침이 필요하므로 이를 강조합니다.
            drawBounds(window, monster.getCollision().getHitbox(), sf::Color::Red, 2.f);
        } else if (monster.getAttackPattern() == MonsterAttackPattern::ChargeCombo &&
            monster.state == MonsterState::Charge) {
            // 돌진 공격의 실제 판정은 몸체 충돌이므로 주황색 상자로 표시합니다.
            drawBounds(window, monster.getCollision().getHitbox(), sf::Color(255, 160, 0), 2.f);
        }
    });
}

bool DebugManager::buildRoomPreviews(const std::vector<const Room*>& rooms,
    const std::string& tileAtlasKey, const RoomTileSet& tileSet,
    sf::Vector2u viewportSize) {
    constexpr float maxPreviewScale = 0.45f;
    constexpr float margin = 28.f;

    m_roomPreviews.clear();
    if (rooms.empty()) {
        return false;
    }

    for (const Room* room : rooms) {
        if (!room) {
            m_roomPreviews.clear();
            return false;
        }

        auto preview = std::make_unique<TileMap>();
        if (!room->buildTileMap(*preview, tileAtlasKey, tileSet)) {
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
        const float availableWidth = std::max(1.f,
            static_cast<float>(viewportSize.x) - margin * (columnCount + 1));
        const float availableHeight = std::max(1.f,
            static_cast<float>(viewportSize.y) - margin * (rowCount + 1));
        const float scale = std::min({ maxPreviewScale,
            availableWidth / totalWidth, availableHeight / totalHeight });
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
        columnPositions[column] = columnPositions[column - 1] +
            columnWidths[column - 1] * bestScale + margin;
    }
    for (std::size_t row = 1; row < rowCount; ++row) {
        rowPositions[row] = rowPositions[row - 1] +
            rowHeights[row - 1] * bestScale + margin;
    }
    for (std::size_t index = 0; index < roomCount; ++index) {
        const std::size_t column = index % bestColumnCount;
        const std::size_t row = index / bestColumnCount;
        TileMap& preview = *m_roomPreviews[index];
        preview.setPosition({ columnPositions[column], rowPositions[row] });
        preview.setScale({ bestScale, bestScale });
    }
    return true;
}

void DebugManager::renderRoomPreviews(sf::RenderWindow& window) const {
    for (const auto& preview : m_roomPreviews) {
        window.draw(*preview);
    }
}
