#include "Room.h"

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <utility>

namespace {
struct PlatformSpec {
    unsigned int x;
    unsigned int y;
    unsigned int length;
};

/// DoorPosition 열거형을 DoorPositions 배열의 인덱스로 변환해 해당 방향의 문 활성 여부를 읽습니다.
bool hasDoor(const DoorPositions& positions, DoorPosition position) {
    return positions[static_cast<std::size_t>(position)];
}

/// 기본 방 레이아웃의 상·하·좌·우 벽을 문 방향 배열에 따라 3타일 폭/높이의 보행 가능한 확장 통로로 바꿉니다.
/// 문 주위의 코너와 외벽도 함께 배치해 통로 바깥으로 플레이어가 빠지지 않게 합니다.
void applyDoorways(RoomLayout& layout, const DoorPositions& positions) {
    const unsigned int width = layout.width;
    const unsigned int height = layout.height;
    const unsigned int outlineWidth = layout.outlineWidth;
    const unsigned int topBottomPassageWidth = layout.topBottomPassageWidth;
    const unsigned int sidePassageHeight = layout.sidePassageHeight;
    if (outlineWidth == 0 || topBottomPassageWidth == 0 || sidePassageHeight == 0 ||
        width < 2 * outlineWidth + topBottomPassageWidth ||
        height < 2 * outlineWidth + sidePassageHeight + 1) {
        return;  
    }

    const auto setCell = [&](unsigned int x, unsigned int y, RoomCell cell) {
        layout.cells[x + y * width] = cell;
    };
    const unsigned int centerStart = (width - topBottomPassageWidth) / 2;
    const unsigned int topWallY = outlineWidth;
    const unsigned int groundY = height - outlineWidth - 1;
    const unsigned int sideDoorTop = groundY - sidePassageHeight;

    if (hasDoor(positions, DoorPosition::Up)) {
        const unsigned int leftEdge = centerStart - 1;
        const unsigned int rightEdge = centerStart + topBottomPassageWidth;
        setCell(leftEdge, 0, RoomCell::TopLeftCorner);
        for (unsigned int x = centerStart;
            x < centerStart + topBottomPassageWidth; ++x) {
            setCell(x, 0, RoomCell::Ceiling);
            setCell(x, topWallY - 1, RoomCell::Door);
        }
        setCell(rightEdge, 0, RoomCell::TopRightCorner);
        setCell(leftEdge, topWallY - 1, RoomCell::LeftWall);
        setCell(rightEdge, topWallY - 1, RoomCell::RightWall);
        setCell(leftEdge, topWallY, RoomCell::DoorUpLeftCorner);
        setCell(rightEdge, topWallY, RoomCell::DoorUpRightCorner);
        setCell(centerStart, topWallY + 1, RoomCell::BackDoorTopLeft);
        setCell(centerStart + topBottomPassageWidth - 1, topWallY + 1,
            RoomCell::BackDoorTopRight);

        for (unsigned int x = centerStart;
            x < centerStart + topBottomPassageWidth; ++x) {
            setCell(x, topWallY, RoomCell::Door);
        }
    }

    if (hasDoor(positions, DoorPosition::Down)) {
        const unsigned int leftEdge = centerStart - 1;
        const unsigned int rightEdge = centerStart + topBottomPassageWidth;
        setCell(leftEdge, height - 1, RoomCell::BottomLeftCorner);
        for (unsigned int x = centerStart;
            x < centerStart + topBottomPassageWidth; ++x) {
            setCell(x, groundY, RoomCell::Door);
            setCell(x, groundY + 1, RoomCell::Door);
            setCell(x, height - 1, RoomCell::Ground);
        }
        setCell(rightEdge, height - 1, RoomCell::BottomRightCorner);
        setCell(leftEdge, groundY + 1, RoomCell::LeftWall);
        setCell(rightEdge, groundY + 1, RoomCell::RightWall);
        setCell(leftEdge, groundY, RoomCell::DoorDownLeftCorner);
        setCell(rightEdge, groundY, RoomCell::DoorDownRightCorner);
        setCell(centerStart, groundY - 1, RoomCell::BackDoorBottomLeft);
        setCell(centerStart + topBottomPassageWidth - 1, groundY - 1,
            RoomCell::BackDoorBottomRight);
    }

    if (hasDoor(positions, DoorPosition::Left)) {
        setCell(0, sideDoorTop - 1, RoomCell::TopLeftCorner);
        setCell(1, sideDoorTop - 1, RoomCell::Ceiling);
        setCell(outlineWidth, sideDoorTop - 1, RoomCell::DoorLeftCorner);

        for (unsigned int y = sideDoorTop; y < groundY; ++y) {
            for (unsigned int x = 0; x <= outlineWidth; ++x) {
                setCell(x, y, RoomCell::Door);
            }
        }
        setCell(0, groundY, RoomCell::BottomLeftCorner);
        for (unsigned int x = 1; x <= outlineWidth; ++x) {
            setCell(x, groundY, RoomCell::Ground);
        }
        setCell(outlineWidth + 1, sideDoorTop,
            RoomCell::BackDoorTopLeft);

        for (unsigned int y = groundY - sidePassageHeight; y < groundY; ++y) {
            setCell(0, y, RoomCell::LeftWall);
        }
    }

    if (hasDoor(positions, DoorPosition::Right)) {
        const unsigned int rightWallX = width - outlineWidth - 1;
        setCell(rightWallX, sideDoorTop - 1, RoomCell::DoorRightCorner);
        setCell(width - 2, sideDoorTop - 1, RoomCell::Ceiling);
        setCell(width - 1, sideDoorTop - 1, RoomCell::TopRightCorner);

        for (unsigned int y = sideDoorTop; y < groundY; ++y) {
            for (unsigned int x = rightWallX; x < width; ++x) {
                setCell(x, y, RoomCell::Door);
            }
        }
        for (unsigned int x = rightWallX; x < width - 1; ++x) {
            setCell(x, groundY, RoomCell::Ground);
        }
        setCell(width - 1, groundY, RoomCell::BottomRightCorner);
        setCell(rightWallX - 1, sideDoorTop,
            RoomCell::BackDoorTopRight);
        for (unsigned int y = groundY - sidePassageHeight; y < groundY; ++y) {
            setCell(width-1, y, RoomCell::RightWall);
        }
    }
}
/// 지정 크기의 기본 방을 만들고, 벽·백타일·스폰 위치 및 길이 3~5의 공중 플랫폼을 배치합니다.
/// 실제 문 확장 처리는 레이아웃 생성 후 applyDoorways가 담당합니다.
RoomLayout makeStyledRoom(unsigned int width, unsigned int height,
    std::initializer_list<PlatformSpec> platforms)
{
    RoomLayout layout;
    layout.width = width;
    layout.height = height;
    layout.cells.assign(width * height, RoomCell::Empty);
    const unsigned int outlineWidth = layout.outlineWidth;

    if (width < 2 * outlineWidth + 1 || height < 2 * outlineWidth + 1) {
        return layout;
    }

    const auto setCell = [&](unsigned int x, unsigned int y, RoomCell cell) {
        layout.cells[x + y * width] = cell;
    };
    const unsigned int leftWallX = outlineWidth;
    const unsigned int rightWallX = width - outlineWidth - 1;
    const unsigned int topWallY = outlineWidth;
    const unsigned int groundY = height - outlineWidth - 1;

    for (unsigned int x = leftWallX; x <= rightWallX; ++x) {
        setCell(x, topWallY, RoomCell::Ceiling);
    }
    setCell(leftWallX, topWallY, RoomCell::TopLeftCorner);
    setCell(rightWallX, topWallY, RoomCell::TopRightCorner);

    // 좌우 문 확장 공간 아래까지 바닥을 이어서, 문 바깥으로 플레이어가 떨어지지 않게 합니다.
    for (unsigned int x = outlineWidth; x < width; ++x) {
        setCell(x, groundY, RoomCell::Ground);
    }
    setCell(leftWallX, groundY, RoomCell::BottomLeftCorner);
    setCell(rightWallX, groundY, RoomCell::BottomRightCorner);

    for (unsigned int y = topWallY + 1; y < groundY; ++y) {
        setCell(leftWallX, y, RoomCell::LeftWall);
        setCell(rightWallX, y, RoomCell::RightWall);
        for (unsigned int x = leftWallX + 1; x < rightWallX; ++x) {
            setCell(x, y, RoomCell::BackTile);
        }
    }

    layout.playerSpawnCell = sf::Vector2u{ leftWallX + 4, groundY - 1 };

    for (const PlatformSpec& platform : platforms) {
        for (unsigned int offset = 0; offset < platform.length; ++offset) {
            const unsigned int x = platform.x + offset;
            if (x < width && platform.y < height &&
                layout.cells[x + platform.y * width] == RoomCell::BackTile) {
                setCell(x, platform.y, RoomCell::Platform);
            }
        }
    }
    return layout;
}
}

/// 생성 시 바로 레퍼런스 레이아웃을 불러와 Room 객체가 항상 유효한 방 정보를 갖게 합니다.
Room::Room(RoomType type, DoorPositions doorPositions,
    std::vector<RoomMonsterSpawn> monsterSpawns) {
    loadReference(type, doorPositions);
    info.monsterSpawns = std::move(monsterSpawns);
}

/// 기본 레이아웃에 문 확장을 덮어쓴 뒤, 최종 Door 셀과 플레이어 스폰 셀을 RoomInfo에 다시 수집합니다.
void Room::loadReference(RoomType type, DoorPositions doorPositions) {
    loadLayout(type, getReferenceLayout(type), doorPositions);
}

void Room::loadLayout(RoomType type, RoomLayout layout, DoorPositions doorPositions,
    std::vector<DecorativeTileConfig> decorations,
    std::vector<BackgroundLayerConfig> backgroundLayers) {
    info.type = type;
    info.isClear = false;
    info.encounterMonsters.clear();
    info.encounterPhaseCount = 0;
    info.isEncounterInitialized = false;
    info.clearRewardChestOpened = false;
    info.clearRewardCollected = false;
    info.doorPositions = doorPositions;
    info.layout = std::move(layout);
    info.decorations = std::move(decorations);
    info.backgroundLayers = std::move(backgroundLayers);
    applyDoorways(info.layout, doorPositions);
    info.doors.resize(4);
    for (std::size_t index = 0; index < info.doors.size(); ++index) {
        info.doors[index].isOpen = doorPositions[index];
        info.doors[index].next = nullptr;
    }
    info.playerSpawnCell = info.layout.playerSpawnCell;
    for (unsigned int y = 0; y < info.layout.height; ++y) {
        for (unsigned int x = 0; x < info.layout.width; ++x) {
            if (!info.playerSpawnCell &&
                info.layout.cells[x + y * info.layout.width] == RoomCell::SpawnPoint) {
                // 이전 레이아웃 데이터와의 호환을 위해서만 SpawnPoint 셀을 읽습니다.
                info.playerSpawnCell = { x, y };
            }
        }
    }
}
void Room::setMonsterSpawns(std::vector<RoomMonsterSpawn> monsterSpawns) {
    info.monsterSpawns = std::move(monsterSpawns);
    info.encounterMonsters.clear();
    info.encounterPhaseCount = 0;
    info.isEncounterInitialized = false;
    info.isClear = false;
}

void Room::setMonsterPhaseConfig(RoomMonsterPhaseConfig config) {
    info.monsterPhaseConfig = std::move(config);
    info.encounterMonsters.clear();
    info.encounterPhaseCount = 0;
    info.isEncounterInitialized = false;
    info.isClear = false;
    info.traversalLockOverride = false;
}

void Room::setClearRewardConfig(RoomClearRewardConfig config) {
    info.clearReward = config;
    info.clearRewardChestOpened = false;
    info.clearRewardCollected = false;
}

void Room::prepareMonsterEncounter(std::vector<RoomMonsterSpawn> monsters,
    int phaseCount) {
    info.encounterMonsters = std::move(monsters);
    info.encounterPhaseCount = std::max(phaseCount, 1);
    info.isEncounterInitialized = true;
    info.isClear = info.encounterMonsters.empty();
}

void Room::setClear(bool isClear) {
    info.isClear = isClear;
}

void Room::setTraversalLocked(bool locked) {
    info.traversalLockOverride = locked;
}

bool Room::isTraversalLocked() const {
    return info.traversalLockOverride ||
        (info.isEncounterInitialized && !info.isClear);
}

std::optional<DoorPosition> Room::getEnteredDoor(
    const sf::FloatRect& actorBounds,
    const sf::FloatRect& previousActorBounds,
    const TileMap& tileMap) const {
    const sf::Vector2f tileSize = tileMap.getTileSize();
    const unsigned int outlineWidth = info.layout.outlineWidth;
    const unsigned int topBottomPassageWidth = info.layout.topBottomPassageWidth;
    const unsigned int sidePassageHeight = info.layout.sidePassageHeight;
    if (tileSize.x <= 0.f || tileSize.y <= 0.f ||
        outlineWidth == 0 || topBottomPassageWidth == 0 || sidePassageHeight == 0 ||
        info.layout.width < 2 * outlineWidth + topBottomPassageWidth ||
        info.layout.height < 2 * outlineWidth + sidePassageHeight + 1) {
        return std::nullopt;
    }

    const unsigned int centerStart =
        (info.layout.width - topBottomPassageWidth) / 2;
    const unsigned int groundY =
        info.layout.height - outlineWidth - 1;
    const unsigned int sideDoorTop = groundY - sidePassageHeight;
    const unsigned int rightWallX =
        info.layout.width - outlineWidth - 1;

    const auto makeBounds = [&tileSize](unsigned int x, unsigned int y,
        unsigned int width, unsigned int height) {
        return sf::FloatRect(
            { static_cast<float>(x) * tileSize.x,
              static_cast<float>(y) * tileSize.y },
            { static_cast<float>(width) * tileSize.x,
              static_cast<float>(height) * tileSize.y });
    };
    const std::array<std::pair<DoorPosition, sf::FloatRect>, 4> triggers{
        std::pair{ DoorPosition::Up,
            makeBounds(centerStart, outlineWidth - 1,
                topBottomPassageWidth, 2) },
        std::pair{ DoorPosition::Down,
            makeBounds(centerStart, groundY, topBottomPassageWidth, 2) },
        std::pair{ DoorPosition::Left,
            makeBounds(0, sideDoorTop, outlineWidth + 1,
                sidePassageHeight) },
        std::pair{ DoorPosition::Right,
            makeBounds(rightWallX, sideDoorTop,
                info.layout.width - rightWallX, sidePassageHeight) }
    };

    const auto intersectsMovement =
        [&](const sf::FloatRect& triggerBounds) {
        if (actorBounds.findIntersection(triggerBounds) ||
            previousActorBounds.findIntersection(triggerBounds)) {
            return true;
        }

        const sf::Vector2f start{
            previousActorBounds.position.x +
                previousActorBounds.size.x * 0.5f,
            previousActorBounds.position.y +
                previousActorBounds.size.y * 0.5f
        };
        const sf::Vector2f end{
            actorBounds.position.x + actorBounds.size.x * 0.5f,
            actorBounds.position.y + actorBounds.size.y * 0.5f
        };
        const float halfWidth = std::max(
            actorBounds.size.x, previousActorBounds.size.x) * 0.5f;
        const float halfHeight = std::max(
            actorBounds.size.y, previousActorBounds.size.y) * 0.5f;
        const sf::FloatRect expandedTrigger{
            { triggerBounds.position.x - halfWidth,
              triggerBounds.position.y - halfHeight },
            { triggerBounds.size.x + halfWidth * 2.f,
              triggerBounds.size.y + halfHeight * 2.f }
        };
        const sf::Vector2f delta = end - start;
        float minimumTime = 0.f;
        float maximumTime = 1.f;

        const auto intersectsAxis = [&](float origin, float direction,
            float minimum, float maximum) {
            constexpr float epsilon = 0.0001f;
            if (std::abs(direction) <= epsilon) {
                return origin >= minimum && origin <= maximum;
            }

            float entryTime = (minimum - origin) / direction;
            float exitTime = (maximum - origin) / direction;
            if (entryTime > exitTime) {
                std::swap(entryTime, exitTime);
            }
            minimumTime = std::max(minimumTime, entryTime);
            maximumTime = std::min(maximumTime, exitTime);
            return minimumTime <= maximumTime;
        };

        return intersectsAxis(start.x, delta.x,
                   expandedTrigger.position.x,
                   expandedTrigger.position.x +
                       expandedTrigger.size.x) &&
            intersectsAxis(start.y, delta.y,
                expandedTrigger.position.y,
                expandedTrigger.position.y +
                    expandedTrigger.size.y);
    };

    for (const auto& [position, triggerBounds] : triggers) {
        const std::size_t index = static_cast<std::size_t>(position);
        if (index < info.doors.size() &&
            info.doors[index].isOpen &&
            info.doors[index].next &&
            intersectsMovement(triggerBounds)) {
            return position;
        }
    }
    return std::nullopt;
}
void Room::setDoorNext(DoorPosition position, Room* nextRoom) {
    const std::size_t index = static_cast<std::size_t>(position);
    if (nextRoom != this && index < info.doors.size() &&
        (!info.doors[index].next || info.doors[index].next == nextRoom)) {
        info.doors[index].next = nextRoom;
    }
}

/// 활성화된 문에 연결된 Room 객체를 반환해, 비활성 방향의 전환을 방지합니다.
Room* Room::getDoorNext(DoorPosition position) const {
    const std::size_t index = static_cast<std::size_t>(position);
    if (index >= info.doors.size() || !info.doors[index].isOpen) {
        return nullptr;
    }
    return info.doors[index].next;
}

/// 방 셀의 논리 타입을 타일 프레임·충돌 타입으로 바꾸며, 주변 벽을 검사해 알맞은 그림자 백타일도 선택합니다.
bool Room::buildTileMap(TileMap& tileMap, const std::string& tileAtlasKey,
    const RoomTileSet& tileSet) const
{
    if (info.layout.width == 0 || info.layout.height == 0 ||
        info.layout.cells.size() != info.layout.width * info.layout.height) {
        return false;
    }

    // 범위 밖 셀을 Empty로 취급해 가장자리에서 주변 타일을 안전하게 검사합니다.
    const auto getCell = [&](int x, int y) {
        if (x < 0 || y < 0 || x >= static_cast<int>(info.layout.width) ||
            y >= static_cast<int>(info.layout.height)) {
            return RoomCell::Empty;
        }
        return info.layout.cells[static_cast<unsigned int>(x) +
            static_cast<unsigned int>(y) * info.layout.width];
    };
    // 백타일은 시각 전용이므로 충돌 없이 배경 버텍스 배열에 넣습니다.
    const auto makeBackTile = [](const std::string& frameName) {
        return TileConfig{ frameName, TileType::None, true };
    };
    // 벽과 맞닿는 방향 및 문 입구의 모서리를 기준으로 적합한 그림자 백타일 프레임을 선택합니다.
    const auto getBackFrame = [&](unsigned int x, unsigned int y, RoomCell cell)
        -> const std::string&
    {
        const unsigned int outlineWidth = info.layout.outlineWidth;
        const unsigned int topBottomPassageWidth = info.layout.topBottomPassageWidth;
        const unsigned int sidePassageHeight = info.layout.sidePassageHeight;
        const unsigned int topWallY = outlineWidth;
        const unsigned int groundY = info.layout.height - outlineWidth - 1;
        const unsigned int sideDoorTop = groundY - sidePassageHeight;
        const unsigned int centerStart =
            (info.layout.width - topBottomPassageWidth) / 2;
        const unsigned int centerEnd = centerStart + topBottomPassageWidth - 1;

        if (cell == RoomCell::Door) {
            const auto isWall = [](RoomCell neighbor) {
                return neighbor == RoomCell::Ceiling ||
                    neighbor == RoomCell::Ground ||
                    neighbor == RoomCell::LeftWall ||
                    neighbor == RoomCell::RightWall ||
                    neighbor == RoomCell::TopLeftCorner ||
                    neighbor == RoomCell::TopRightCorner ||
                    neighbor == RoomCell::BottomLeftCorner ||
                    neighbor == RoomCell::BottomRightCorner ||
                    neighbor == RoomCell::DoorUpLeftCorner ||
                    neighbor == RoomCell::DoorUpRightCorner ||
                    neighbor == RoomCell::DoorDownLeftCorner ||
                    neighbor == RoomCell::DoorDownRightCorner ||
                    neighbor == RoomCell::DoorLeftCorner ||
                    neighbor == RoomCell::DoorRightCorner;
            };
            const bool topWall = isWall(getCell(static_cast<int>(x), static_cast<int>(y) - 1));
            const bool bottomWall = isWall(getCell(static_cast<int>(x), static_cast<int>(y) + 1));
            const bool leftWall = isWall(getCell(static_cast<int>(x) - 1, static_cast<int>(y)));
            const bool rightWall = isWall(getCell(static_cast<int>(x) + 1, static_cast<int>(y)));

            if (topWall && leftWall) return tileSet.backTopLeftCornerFrameName;
            if (topWall && rightWall) return tileSet.backTopRightCornerFrameName;
            if (bottomWall && leftWall) return tileSet.backBottomLeftCornerFrameName;
            if (bottomWall && rightWall) return tileSet.backBottomRightCornerFrameName;
            if (topWall) return tileSet.backTopFrameName;
            if (bottomWall) return tileSet.backGroundFrameName;
            if (leftWall) return tileSet.backLeftFrameName;
            if (rightWall) return tileSet.backRightFrameName;
            return tileSet.backInnerFrameName;
        }

        // 문은 방 안쪽 첫 백타일도 바꿉니다. 이 셀들에 문 입구 방향의 한쪽 그림자가 생깁니다.
        if (hasDoor(info.doorPositions, DoorPosition::Up) && y == topWallY + 1) {
            // 상단 문의 양 바깥 경계는 천장과 이어지는 일반 백타일을 유지합니다.
            if (x == centerStart - 1 || x == centerEnd + 1) {
                return tileSet.backTopFrameName;
            }
            // 실제 입구와 맞닿는 양끝만 위쪽 문 그림자 타일을 사용합니다.
            if (x == centerStart) return tileSet.backDoorTopLeftFrameName;
            if (x == centerEnd) return tileSet.backDoorTopRightFrameName;
            if (x > centerStart && x < centerEnd) return tileSet.backInnerFrameName;
        }
        if (hasDoor(info.doorPositions, DoorPosition::Down) && y == groundY - 1) {
            if (x == centerStart - 1 || x == centerEnd + 1) {
                return tileSet.backGroundFrameName;
            }
            if (x == centerStart) return tileSet.backDoorBottomLeftFrameName;
            if (x == centerEnd) return tileSet.backDoorBottomRightFrameName;
            if (x > centerStart && x < centerEnd) return tileSet.backInnerFrameName;
        }
        if (hasDoor(info.doorPositions, DoorPosition::Left) && x == outlineWidth + 1) {
            // 입구 위쪽의 방 내부 경계는 좌측 벽과 연결되는 일반 백타일입니다.
            if (y == sideDoorTop - 1) return tileSet.backLeftFrameName;
            if (y == sideDoorTop) return tileSet.backDoorTopLeftFrameName;
            if (y == groundY - 1) return tileSet.backGroundFrameName;
            if (y > sideDoorTop && y < groundY - 1) return tileSet.backInnerFrameName;
        }
        const unsigned int rightInnerX = info.layout.width - outlineWidth - 2;
        if (hasDoor(info.doorPositions, DoorPosition::Right) && x == rightInnerX) {
            // 입구 위쪽의 방 내부 경계는 우측 벽과 연결되는 일반 백타일입니다.
            if (y == sideDoorTop - 1) return tileSet.backRightFrameName;
            if (y == sideDoorTop) return tileSet.backDoorTopRightFrameName;
            if (y == groundY - 1) return tileSet.backGroundFrameName;
            if (y > sideDoorTop && y < groundY - 1) return tileSet.backInnerFrameName;
        }

        const RoomCell left = getCell(static_cast<int>(x) - 1, static_cast<int>(y));
        const RoomCell right = getCell(static_cast<int>(x) + 1, static_cast<int>(y));
        const RoomCell top = getCell(static_cast<int>(x), static_cast<int>(y) - 1);
        const RoomCell bottom = getCell(static_cast<int>(x), static_cast<int>(y) + 1);
        const bool leftWall = left == RoomCell::LeftWall;
        const bool rightWall = right == RoomCell::RightWall;
        const bool topWall = top == RoomCell::Ceiling || top == RoomCell::TopLeftCorner ||
            top == RoomCell::TopRightCorner;
        const bool bottomWall = bottom == RoomCell::Ground ||
            bottom == RoomCell::BottomLeftCorner || bottom == RoomCell::BottomRightCorner;

        if (topWall && leftWall) return tileSet.backTopLeftCornerFrameName;
        if (topWall && rightWall) return tileSet.backTopRightCornerFrameName;
        if (bottomWall && leftWall) return tileSet.backBottomLeftCornerFrameName;
        if (bottomWall && rightWall) return tileSet.backBottomRightCornerFrameName;
        if (topWall) return tileSet.backTopFrameName;
        if (bottomWall) return tileSet.backGroundFrameName;
        if (leftWall) return tileSet.backLeftFrameName;
        if (rightWall) return tileSet.backRightFrameName;
        return tileSet.backInnerFrameName;
    };

    std::vector<TileConfig> grid(info.layout.cells.size());
    for (std::size_t index = 0; index < info.layout.cells.size(); ++index) {
        const unsigned int x = static_cast<unsigned int>(index % info.layout.width);
        const unsigned int y = static_cast<unsigned int>(index / info.layout.width);
        TileConfig& config = grid[index];
        switch (info.layout.cells[index]) {
        case RoomCell::Ceiling:
            config = { tileSet.wallTopFrameName, TileType::Solid };
            break;
        case RoomCell::TopLeftCorner:
            config = { tileSet.topLeftCornerFrameName, TileType::Solid };
            break;
        case RoomCell::TopRightCorner:
            config = { tileSet.topRightCornerFrameName, TileType::Solid };
            break;
        case RoomCell::Ground:
            config = { tileSet.wallGroundFrameName, TileType::Solid };
            break;
        case RoomCell::BottomLeftCorner:
            config = { tileSet.bottomLeftCornerFrameName, TileType::Solid };
            break;
        case RoomCell::BottomRightCorner:
            config = { tileSet.bottomRightCornerFrameName, TileType::Solid };
            break;
        case RoomCell::DoorUpLeftCorner:
            config = { tileSet.wallDoorBottomLeftFrameName, TileType::Solid };
            break;
        case RoomCell::DoorUpRightCorner:
            config = { tileSet.wallDoorBottomRightFrameName, TileType::Solid };
            break;
        case RoomCell::DoorDownLeftCorner:
            config = { tileSet.wallDoorTopLeftFrameName, TileType::Solid };
            break;
        case RoomCell::DoorDownRightCorner:
            config = { tileSet.wallDoorTopRightFrameName, TileType::Solid };
            break;
        case RoomCell::DoorLeftCorner:
            config = { tileSet.wallDoorTopRightFrameName, TileType::Solid };
            break;
        case RoomCell::DoorRightCorner:
            config = { tileSet.wallDoorTopLeftFrameName, TileType::Solid };
            break;
        case RoomCell::BackDoorTopLeft:
            config = makeBackTile(tileSet.backDoorTopLeftFrameName);
            break;
        case RoomCell::BackDoorTopRight:
            config = makeBackTile(tileSet.backDoorTopRightFrameName);
            break;
        case RoomCell::BackDoorBottomLeft:
            config = makeBackTile(tileSet.backDoorBottomLeftFrameName);
            break;
        case RoomCell::BackDoorBottomRight:
            config = makeBackTile(tileSet.backDoorBottomRightFrameName);
            break;
        case RoomCell::LeftWall:
            config = { tileSet.wallLeftFrameName, TileType::Solid };
            break;
        case RoomCell::RightWall:
            config = { tileSet.wallRightFrameName, TileType::Solid };
            break;
        case RoomCell::Platform:
            config = { tileSet.platformFrameName, TileType::OneWay };
            break;
        case RoomCell::BackTile:
            config = makeBackTile(getBackFrame(x, y, info.layout.cells[index]));
            break;
        case RoomCell::SpawnPoint:
            // 스폰 표시는 논리 정보만 추가
            break;
        case RoomCell::OpenSpace:
            // 시작마을의 배경 위에서는 별도 타일을 렌더링하지 않습니다.
            break;
        case RoomCell::InvisibleWall:
            config = { tileSet.backInnerFrameName, TileType::Solid, false, 0, false };
            break;
        case RoomCell::Door:
            // 문은 걸을 수 있지만 시각적으로는 방 내부이므로 백타일로 렌더링합니다.
            config = makeBackTile(getBackFrame(x, y, RoomCell::Door));
            break;
        case RoomCell::Empty:
            // 방 기준 레이아웃의 외곽 여백은 외부 벽으로 채워 통로 밖 이탈을 막습니다.
            config = { tileSet.WallOutterFrameName, TileType::Solid };
            break;
        }

    }
    if (!tileMap.load(tileAtlasKey, grid, info.layout.width, info.layout.height,
        info.decorations)) {
        return false;
    }
    if (!info.backgroundLayers.empty()) {
        tileMap.setBackgroundLayers(info.backgroundLayers);
    }

    const sf::Vector2f tileSize = tileMap.getTileSize();
    const unsigned int outlineWidth = info.layout.outlineWidth;
    const unsigned int topBottomPassageWidth = info.layout.topBottomPassageWidth;
    const unsigned int sidePassageHeight = info.layout.sidePassageHeight;
    const unsigned int centerStart =
        (info.layout.width - topBottomPassageWidth) / 2;
    const unsigned int groundY = info.layout.height - outlineWidth - 1;
    const unsigned int sideDoorTop = groundY - sidePassageHeight;
    const unsigned int rightWallX = info.layout.width - outlineWidth - 1;
    const float topPassageInnerY = (outlineWidth + 1.f) * tileSize.y;
    const float bottomPassageInnerY = groundY * tileSize.y;
    const float leftPassageInnerX = (outlineWidth + 1.f) * tileSize.x;
    const float rightPassageInnerX = rightWallX * tileSize.x;
    std::vector<DoorAnimationPlacement> doorPlacements;
    doorPlacements.reserve(4);

    // 각 문 스프라이트의 위쪽이 항상 방 안쪽을 향하도록 회전합니다.
    if (hasDoor(info.doorPositions, DoorPosition::Up)) {
        doorPlacements.push_back({
            { (centerStart + topBottomPassageWidth * 0.5f) * tileSize.x,
                topPassageInnerY },
            180.f
        });
    }
    if (hasDoor(info.doorPositions, DoorPosition::Down)) {
        doorPlacements.push_back({
            { (centerStart + topBottomPassageWidth * 0.5f) * tileSize.x,
                bottomPassageInnerY },
            0.f
        });
    }
    if (hasDoor(info.doorPositions, DoorPosition::Left)) {
        doorPlacements.push_back({
            { leftPassageInnerX,
                (sideDoorTop + sidePassageHeight * 0.5f) * tileSize.y },
            90.f
        });
    }
    if (hasDoor(info.doorPositions, DoorPosition::Right)) {
        doorPlacements.push_back({
            { rightPassageInnerX,
                (sideDoorTop + sidePassageHeight * 0.5f) * tileSize.y },
            -90.f
        });
    }
    return tileMap.configureDoorAnimations(tileAtlasKey, doorPlacements);
}

/// 논리 스폰 셀의 발밑 중심을 반환해 Actor의 bottom-center 원점과 맞춥니다.
std::optional<sf::Vector2f> Room::getPlayerSpawnPosition(const TileMap& tileMap) const {
    if (!info.playerSpawnCell || tileMap.getTileSize().x <= 0.f || tileMap.getTileSize().y <= 0.f) {
        return std::nullopt;
    }

    const sf::Vector2f tileSize = tileMap.getTileSize();
    return sf::Vector2f{
        (static_cast<float>(info.playerSpawnCell->x) + 0.5f) * tileSize.x,
        (static_cast<float>(info.playerSpawnCell->y) + 1.f) * tileSize.y
    };
}

/// 발아래가 바닥 또는 플랫폼인 백타일만 후보로 모아, 가운데 후보를 몬스터 스폰 위치로 선택합니다.
std::vector<sf::Vector2f> Room::getMonsterSpawnPositions(const TileMap& tileMap) const {
    const sf::Vector2f tileSize = tileMap.getTileSize();
    std::vector<sf::Vector2f> positions;
    if (tileSize.x <= 0.f || tileSize.y <= 0.f || info.layout.height < 2) {
        return positions;
    }

    for (unsigned int y = 1; y < info.layout.height - 1; ++y) {
        for (unsigned int x = 1; x < info.layout.width - 1; ++x) {
            const unsigned int index = x + y * info.layout.width;
            const unsigned int belowIndex = x + (y + 1) * info.layout.width;
            const RoomCell cell = info.layout.cells[index];
            const RoomCell below = info.layout.cells[belowIndex];
            if ((cell == RoomCell::BackTile || cell == RoomCell::SpawnPoint) &&
                (below == RoomCell::Ground || below == RoomCell::Platform)) {
                positions.push_back({
                    (static_cast<float>(x) + 0.5f) * tileSize.x,
                    (static_cast<float>(y) + 1.f) * tileSize.y
                });
            }
        }
    }
    return positions;
}
std::optional<sf::Vector2f> Room::getMonsterSpawnPosition(const TileMap& tileMap) const {
    const std::vector<sf::Vector2f> positions = getMonsterSpawnPositions(tileMap);
    if (positions.empty()) {
        return std::nullopt;
    }
    return positions[positions.size() / 2];
}
/// 방 종류별 플랫폼 배치를 고정 데이터로 정의합니다. 보스방은 일반 방의 두 배 폭을 사용합니다.
RoomLayout Room::getReferenceLayout(RoomType type) {
    constexpr unsigned int kRegularRoomWidth = 28;
    constexpr unsigned int kRoomHeight = 20;

    switch (type) {
    case RoomType::Start:
        return makeStyledRoom(kRegularRoomWidth, kRoomHeight,
            { { 11, 6, 3 }, { 6, 9, 3 }, { 17, 9, 3 } });
    case RoomType::Monster:
        return makeStyledRoom(kRegularRoomWidth, kRoomHeight,
            { { 10, 5, 5 }, { 6, 8, 3 }, { 18, 8, 3 }, { 11, 10, 4 } });
    case RoomType::Trial:
        return makeStyledRoom(kRegularRoomWidth, kRoomHeight,
            { { 6, 4, 3 }, { 18, 6, 3 }, { 7, 8, 3 }, { 18, 10, 3 } });
    case RoomType::Monster2:
        return makeStyledRoom(kRegularRoomWidth, kRoomHeight,
            { { 11, 4, 3 }, { 6, 7, 3 }, { 17, 9, 3 } });
    case RoomType::Monster3:
        return makeStyledRoom(kRegularRoomWidth, kRoomHeight,
            { { 6, 4, 3 }, { 18, 6, 3 }, { 10, 8, 5 }, { 6, 10, 3 }, { 18, 10, 3 } });
    case RoomType::Monster4:
        return makeStyledRoom(kRegularRoomWidth, kRoomHeight,
            { { 11, 5, 3 }, { 6, 7, 3 }, { 19, 7, 3 }, { 11, 9, 3 }, { 6, 11, 3 }, { 19, 11, 3 } });
    case RoomType::Monster5:
        return makeStyledRoom(kRegularRoomWidth, kRoomHeight,
            { { 7, 5, 3 }, { 18, 5, 3 }, { 10, 7, 5 }, { 7, 9, 3 }, { 18, 9, 3 } });
    case RoomType::Hut:
        return makeStyledRoom(kRegularRoomWidth, kRoomHeight,
            { { 10, 5, 5 }, { 6, 7, 5 }, { 13, 7, 5 }, { 12, 10, 3 } });
    case RoomType::Boss:
        return makeStyledRoom(kRegularRoomWidth * 2, kRoomHeight, {});
    }
    return {};
}
