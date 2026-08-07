#include "TileMap.h"
#include <array>
#include <iostream>

bool TileMap::load(const std::string& tileAtlasKey, const std::vector<TileConfig>& grid,
    unsigned int width, unsigned int height) {
    auto& resMgr = ResourceManager::getInstance();
    m_tileset = resMgr.getAtlasTexture(tileAtlasKey);
    if (!m_tileset) return false;
    if (grid.size() != width * height) return false;
    m_width = width;
    m_height = height;

    // 첫 유효 타일의 sourceSize를 방 전체의 고정 셀 크기로 사용합니다.
    std::optional<sf::Vector2u> sourceSize;
    for (const TileConfig& config : grid) {
        if (!config.frameName.empty()) {
            sourceSize = resMgr.getFrameSourceSize(tileAtlasKey, config.frameName);
            if (!sourceSize) {
                std::cerr << "[TileMap] sourceSize를 찾을 수 없습니다: " << config.frameName << std::endl;
                return false;
            }
            break;
        }
    }
    if (!sourceSize || sourceSize->x == 0 || sourceSize->y == 0) {
        std::cerr << "[TileMap] 유효한 타일 셀 크기가 없습니다." << std::endl;
        return false;
    }
    m_tileSize = {
        static_cast<float>(sourceSize->x), static_cast<float>(sourceSize->y)
    };

    // SFML 3.1.0은 Quads를 지원하지 않으므로 Triangles 사용 (타일 1개당 6개 정점)
    m_vertices.resize(width * height * 6);
    m_backgroundVertices.resize(width * height * 6);
    m_collisionTiles.clear();

    for (unsigned int i = 0; i < width; ++i) {
        for (unsigned int j = 0; j < height; ++j) {
            unsigned int index = i + j * width;
            const TileConfig& config = grid[index];

            const auto appendTile = [&](sf::VertexArray& vertices, const std::string& frameName,
                sf::Color tileColor, int rotationQuarterTurns) {
                const sf::IntRect* rect = resMgr.getFrameRect(tileAtlasKey, frameName);
                if (!rect) {
                    std::cerr << "[TileMap] frame을 찾을 수 없습니다: " << frameName << std::endl;
                    return false;
                }

                sf::Vertex* quad = &vertices[index * 6];
                for (int vertexIndex = 0; vertexIndex < 6; ++vertexIndex) {
                    quad[vertexIndex].color = tileColor;
                }

                quad[0].position = sf::Vector2f(i * m_tileSize.x, j * m_tileSize.y);
                quad[1].position = sf::Vector2f((i + 1) * m_tileSize.x, j * m_tileSize.y);
                quad[2].position = sf::Vector2f(i * m_tileSize.x, (j + 1) * m_tileSize.y);
                quad[3].position = quad[2].position;
                quad[4].position = quad[1].position;
                quad[5].position = sf::Vector2f((i + 1) * m_tileSize.x, (j + 1) * m_tileSize.y);

                const float tx = static_cast<float>(rect->position.x);
                const float ty = static_cast<float>(rect->position.y);
                const float tw = static_cast<float>(rect->size.x);
                const float th = static_cast<float>(rect->size.y);
                const std::array<sf::Vector2f, 4> texCorners{
                    sf::Vector2f(tx, ty),
                    sf::Vector2f(tx + tw, ty),
                    sf::Vector2f(tx, ty + th),
                    sf::Vector2f(tx + tw, ty + th)
                };
                const int turn = (rotationQuarterTurns % 4 + 4) % 4;
                static constexpr std::array<std::array<int, 4>, 4> cornerOrder{
                    std::array<int, 4>{ 0, 1, 2, 3 },
                    std::array<int, 4>{ 2, 0, 3, 1 },
                    std::array<int, 4>{ 3, 2, 1, 0 },
                    std::array<int, 4>{ 1, 3, 0, 2 }
                };
                const std::array<int, 4>& order = cornerOrder[turn];
                quad[0].texCoords = texCorners[order[0]];
                quad[1].texCoords = texCorners[order[1]];
                quad[2].texCoords = texCorners[order[2]];
                quad[3].texCoords = texCorners[order[2]];
                quad[4].texCoords = texCorners[order[1]];
                quad[5].texCoords = texCorners[order[3]];
                return true;
            };

            if (config.frameName.empty()) {
                continue;
            }
            if (config.type != TileType::None) {
                const sf::FloatRect tileBounds(
                    { i * m_tileSize.x, j * m_tileSize.y }, { m_tileSize.x, m_tileSize.y });
                m_collisionTiles.push_back({ tileBounds, config.type });
            }

            sf::VertexArray& vertices = config.isBackground ? m_backgroundVertices : m_vertices;
            const sf::Color tileColor = config.isBackground
                ? sf::Color(145, 145, 145)
                : sf::Color::White;
            if (!appendTile(vertices, config.frameName, tileColor, config.rotationQuarterTurns)) {
                return false;
            }
        }
    }
    return true;
}

void TileMap::setBackground(const std::string& bgAtlasKey, const std::string& bgFrameName) {
    auto& resMgr = ResourceManager::getInstance();
    const sf::Texture* tex = resMgr.getAtlasTexture(bgAtlasKey);
    const sf::IntRect* rect = resMgr.getFrameRect(bgAtlasKey, bgFrameName);

    if (tex && rect) {
        m_background.emplace(*tex);
        m_background->setTextureRect(*rect);
    }
}

void TileMap::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    states.transform *= getTransform();

    // 1. 배경 먼저 렌더링
    if (m_background) {
        target.draw(*m_background, states);
    }

    // 2. 상호작용하지 않는 배경 타일을 먼저 렌더링
    if (m_tileset) {
        states.texture = m_tileset;
        target.draw(m_backgroundVertices, states);

        // 3. 상호작용 타일 렌더링
        target.draw(m_vertices, states);
    }
}
