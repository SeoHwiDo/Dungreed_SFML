#include "Room.h"

#include <initializer_list>

namespace {
struct PlatformSpec {
    unsigned int x;
    unsigned int y;
    unsigned int length;
};

constexpr unsigned int kOutlineWidth = 2;
constexpr unsigned int kDoorHeight = 3;
constexpr unsigned int kCenterDoorWidth = 3;

bool hasDoor(const DoorPositions& positions, DoorPosition position) {
    return positions[static_cast<std::size_t>(position)];
}

void applyDoorways(RoomLayout& layout, const DoorPositions& positions) {
    const unsigned int width = layout.width;
    const unsigned int height = layout.height;
    if (width < 2 * kOutlineWidth + kCenterDoorWidth ||
        height < 2 * kOutlineWidth + kDoorHeight + 1) {
        return;
    }

    const auto setCell = [&](unsigned int x, unsigned int y, RoomCell cell) {
        layout.cells[x + y * width] = cell;
    };
    const unsigned int centerStart = (width - kCenterDoorWidth) / 2;
    const unsigned int topWallY = kOutlineWidth;
    const unsigned int groundY = height - kOutlineWidth - 1;
    const unsigned int sideDoorTop = groundY - kDoorHeight;

    if (hasDoor(positions, DoorPosition::Up)) {
        const unsigned int leftEdge = centerStart - 1;
        const unsigned int rightEdge = centerStart + kCenterDoorWidth;
        setCell(leftEdge, 0, RoomCell::TopLeftCorner);
        for (unsigned int x = centerStart; x < centerStart + kCenterDoorWidth; ++x) {
            setCell(x, 0, RoomCell::Ceiling);
            setCell(x, topWallY - 1, RoomCell::Door);
        }
        setCell(rightEdge, 0, RoomCell::TopRightCorner);
        setCell(leftEdge, topWallY - 1, RoomCell::LeftWall);
        setCell(rightEdge, topWallY - 1, RoomCell::RightWall);

        for (unsigned int x = centerStart; x < centerStart + kCenterDoorWidth; ++x) {
            setCell(x, topWallY, RoomCell::Door);
        }
    }
    if (hasDoor(positions, DoorPosition::Down)) {
        const unsigned int leftEdge = centerStart - 1;
        const unsigned int rightEdge = centerStart + kCenterDoorWidth;
        setCell(leftEdge, height - 1, RoomCell::BottomLeftCorner);
        for (unsigned int x = centerStart; x < centerStart + kCenterDoorWidth; ++x) {
            setCell(x, groundY, RoomCell::Door);
            setCell(x, groundY + 1, RoomCell::Door);
            setCell(x, height - 1, RoomCell::Ground);
        }
        setCell(rightEdge, height - 1, RoomCell::BottomRightCorner);
        setCell(leftEdge, groundY + 1, RoomCell::LeftWall);
        setCell(rightEdge, groundY + 1, RoomCell::RightWall);

    }

    if (hasDoor(positions, DoorPosition::Left)) {
        setCell(0, sideDoorTop - 1, RoomCell::TopLeftCorner);
        setCell(1, sideDoorTop - 1, RoomCell::Ceiling);
        setCell(kOutlineWidth, sideDoorTop - 1, RoomCell::BottomRightCorner);
        setCell(kOutlineWidth, groundY, RoomCell::Ground);

        for (unsigned int y = sideDoorTop; y < groundY; ++y) {
            setCell(0, y, RoomCell::LeftWall);
            setCell(1, y, RoomCell::Door);
            setCell(kOutlineWidth, y, RoomCell::Door);
        }
    }
    if (hasDoor(positions, DoorPosition::Right)) {
        const unsigned int rightWallX = width - kOutlineWidth - 1;
        setCell(rightWallX, sideDoorTop - 1, RoomCell::BottomLeftCorner);
        setCell(width - 2, sideDoorTop - 1, RoomCell::Ceiling);
        setCell(width - 1, sideDoorTop - 1, RoomCell::TopRightCorner);
        setCell(rightWallX, groundY, RoomCell::Ground);

        for (unsigned int y = sideDoorTop; y < groundY; ++y) {
            setCell(rightWallX, y, RoomCell::Door);
            setCell(width - 2, y, RoomCell::Door);
            setCell(width - 1, y, RoomCell::RightWall);
        }
    }
}

RoomLayout makeStyledRoom(unsigned int width, unsigned int height,
    std::initializer_list<PlatformSpec> platforms)
{
    RoomLayout layout;
    layout.width = width;
    layout.height = height;
    layout.cells.assign(width * height, RoomCell::Empty);

    if (width < 2 * kOutlineWidth + 1 || height < 2 * kOutlineWidth + 1) {
        return layout;
    }

    const auto setCell = [&](unsigned int x, unsigned int y, RoomCell cell) {
        layout.cells[x + y * width] = cell;
    };
    const unsigned int leftWallX = kOutlineWidth;
    const unsigned int rightWallX = width - kOutlineWidth - 1;
    const unsigned int topWallY = kOutlineWidth;
    const unsigned int groundY = height - kOutlineWidth - 1;

    for (unsigned int x = leftWallX; x <= rightWallX; ++x) {
        setCell(x, topWallY, RoomCell::Ceiling);
    }
    setCell(leftWallX, topWallY, RoomCell::TopLeftCorner);
    setCell(rightWallX, topWallY, RoomCell::TopRightCorner);

    // The floor extends underneath the side-door protrusions as well.  Without
    // these outer tiles a player could fall out of the doorway extension.
    for (unsigned int x = 0; x < width; ++x) {
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

    // The spawn marker is rendered with the background tile and has no collision.
    setCell(leftWallX + 4, groundY - 1, RoomCell::SpawnPoint);

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

Room::Room(RoomType type, DoorPositions doorPositions) {
    loadReference(type, doorPositions);
}

void Room::loadReference(RoomType type, DoorPositions doorPositions) {
    info.type = type;
    info.isClear = false;
    info.doorPositions = doorPositions;
    info.layout = getReferenceLayout(type);
    applyDoorways(info.layout, doorPositions);
    info.doors.clear();
    info.playerSpawnCell.reset();

    for (unsigned int y = 0; y < info.layout.height; ++y) {
        for (unsigned int x = 0; x < info.layout.width; ++x) {
            const RoomCell cell = info.layout.cells[x + y * info.layout.width];
            if (cell == RoomCell::Door) {
                info.doors.push_back({ { x, y }, false });
            } else if (cell == RoomCell::SpawnPoint) {
                info.playerSpawnCell = { x, y };
            }
        }
    }
}

bool Room::buildTileMap(TileMap& tileMap, const std::string& tileAtlasKey,
    const RoomTileSet& tileSet) const
{
    if (info.layout.width == 0 || info.layout.height == 0 ||
        info.layout.cells.size() != info.layout.width * info.layout.height) {
        return false;
    }

    const auto getCell = [&](int x, int y) {
        if (x < 0 || y < 0 || x >= static_cast<int>(info.layout.width) ||
            y >= static_cast<int>(info.layout.height)) {
            return RoomCell::Empty;
        }
        return info.layout.cells[static_cast<unsigned int>(x) +
            static_cast<unsigned int>(y) * info.layout.width];
    };
    const auto makeBackTile = [](const std::string& frameName) {
        return TileConfig{ frameName, TileType::None, true };
    };
    const auto getBackFrame = [&](unsigned int x, unsigned int y, RoomCell cell)
        -> const std::string&
    {
        const unsigned int topWallY = kOutlineWidth;
        const unsigned int groundY = info.layout.height - kOutlineWidth - 1;
        const unsigned int sideDoorTop = groundY - kDoorHeight;
        const unsigned int centerStart = (info.layout.width - kCenterDoorWidth) / 2;
        const unsigned int centerEnd = centerStart + kCenterDoorWidth - 1;

        if (cell == RoomCell::Door) {
            const auto isWall = [](RoomCell neighbor) {
                return neighbor == RoomCell::Ceiling ||
                    neighbor == RoomCell::Ground ||
                    neighbor == RoomCell::LeftWall ||
                    neighbor == RoomCell::RightWall ||
                    neighbor == RoomCell::TopLeftCorner ||
                    neighbor == RoomCell::TopRightCorner ||
                    neighbor == RoomCell::BottomLeftCorner ||
                    neighbor == RoomCell::BottomRightCorner;
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

        // Door overlays also alter the first background tiles inside the main room.
        // These are the cells that receive the one-sided doorway shadows.
        if (hasDoor(info.doorPositions, DoorPosition::Up) && y == topWallY + 1) {
            if (x == centerStart - 1) return tileSet.backDoorBottomLeftFrameName;
            if (x >= centerStart && x <= centerEnd) return tileSet.backInnerFrameName;
            if (x == centerEnd + 1) return tileSet.backDoorBottomRightFrameName;
        }
        if (hasDoor(info.doorPositions, DoorPosition::Down) && y == groundY - 1) {
            if (x == centerStart - 1 || x == centerEnd + 1) {
                return tileSet.backGroundFrameName;
            }
            if (x == centerStart) return tileSet.backDoorBottomLeftFrameName;
            if (x == centerEnd) return tileSet.backDoorBottomRightFrameName;
            if (x > centerStart && x < centerEnd) return tileSet.backInnerFrameName;
        }
        if (hasDoor(info.doorPositions, DoorPosition::Left) && x == kOutlineWidth + 1 &&
            y >= sideDoorTop && y < groundY) {
            if (y == sideDoorTop) return tileSet.backDoorTopRightFrameName;
            if (y == groundY - 1) return tileSet.backGroundFrameName;
            return tileSet.backInnerFrameName;
        }
        const unsigned int rightInnerX = info.layout.width - kOutlineWidth - 2;
        if (hasDoor(info.doorPositions, DoorPosition::Right) && x == rightInnerX &&
            y >= sideDoorTop && y < groundY) {
            if (y == sideDoorTop) return tileSet.backDoorTopLeftFrameName;
            if (y == groundY - 1) return tileSet.backGroundFrameName;
            return tileSet.backInnerFrameName;
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
        case RoomCell::Ground: {
            const unsigned int centerStart = (info.layout.width - kCenterDoorWidth) / 2;
            const unsigned int centerEnd = centerStart + kCenterDoorWidth;
            const unsigned int groundY = info.layout.height - kOutlineWidth - 1;
            if (hasDoor(info.doorPositions, DoorPosition::Down) && y == groundY &&
                x == centerStart - 1) {
                config = { tileSet.wallDoorCornerLeftFrameName, TileType::Solid };
            } else if (hasDoor(info.doorPositions, DoorPosition::Down) && y == groundY &&
                x == centerEnd) {
                config = { tileSet.wallDoorCornerRightFrameName, TileType::Solid };
            } else {
                config = { tileSet.wallGroundFrameName, TileType::Solid };
            }
            break;
        }
        case RoomCell::BottomLeftCorner:
            config = { tileSet.bottomLeftCornerFrameName, TileType::Solid };
            break;
        case RoomCell::BottomRightCorner:
            config = { tileSet.bottomRightCornerFrameName, TileType::Solid };
            break;
        case RoomCell::LeftWall:
            config = { tileSet.wallLeftFrameName, TileType::Solid };
            break;
        case RoomCell::RightWall:
            config = { tileSet.wallRightFrameName, TileType::Solid };
            break;
        case RoomCell::Platform:
            if (getCell(static_cast<int>(x) - 1, static_cast<int>(y)) != RoomCell::Platform) {
                config = { tileSet.platformLeftFrameName, TileType::OneWay };
            } else if (getCell(static_cast<int>(x) + 1, static_cast<int>(y)) != RoomCell::Platform) {
                config = { tileSet.platformRightFrameName, TileType::OneWay };
            } else {
                config = { tileSet.platformInnerFrameName, TileType::OneWay };
            }
            break;
        case RoomCell::BackTile:
        case RoomCell::SpawnPoint:
            config = makeBackTile(getBackFrame(x, y, info.layout.cells[index]));
            break;
        case RoomCell::Door:
            // A doorway is walkable, but it still belongs to the room visually.
            config = makeBackTile(getBackFrame(x, y, RoomCell::Door));
            break;
        case RoomCell::Empty:
            config = { "", TileType::None };
            break;
        }

    }
    return tileMap.load(tileAtlasKey, grid, info.layout.width, info.layout.height);
}

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

std::optional<sf::Vector2f> Room::getMonsterSpawnPosition(const TileMap& tileMap) const {
    const sf::Vector2f tileSize = tileMap.getTileSize();
    if (tileSize.x <= 0.f || tileSize.y <= 0.f || info.layout.height < 2) {
        return std::nullopt;
    }

    std::vector<sf::Vector2u> candidates;
    for (unsigned int y = 1; y < info.layout.height - 1; ++y) {
        for (unsigned int x = 1; x < info.layout.width - 1; ++x) {
            const unsigned int index = x + y * info.layout.width;
            const unsigned int belowIndex = x + (y + 1) * info.layout.width;
            const RoomCell cell = info.layout.cells[index];
            const RoomCell below = info.layout.cells[belowIndex];
            if ((cell == RoomCell::BackTile || cell == RoomCell::SpawnPoint) &&
                (below == RoomCell::Ground || below == RoomCell::Platform)) {
                candidates.push_back({ x, y });
            }
        }
    }
    if (candidates.empty()) {
        return std::nullopt;
    }

    const sf::Vector2u& cell = candidates[candidates.size() / 2];
    return sf::Vector2f{
        (static_cast<float>(cell.x) + 0.5f) * tileSize.x,
        (static_cast<float>(cell.y) + 1.f) * tileSize.y
    };
}

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
