#include "TileMap.h"
#include <iostream>

bool TileMap::load(const std::string& tileAtlasKey, const std::vector<TileConfig>& grid,
    unsigned int width, unsigned int height, sf::Vector2f tileSize) {
    auto& resMgr = ResourceManager::getInstance();
    m_tileset = resMgr.getAtlasTexture(tileAtlasKey);
    if (!m_tileset) return false;

    // SFML 3.1.0은 Quads를 지원하지 않으므로 Triangles 사용 (타일 1개당 6개 정점)
    m_vertices.resize(width * height * 6);
    m_collisionTiles.clear();

    for (unsigned int i = 0; i < width; ++i) {
        for (unsigned int j = 0; j < height; ++j) {
            unsigned int index = i + j * width;
            const TileConfig& config = grid[index];

            if (config.type == TileType::None || config.frameName.empty()) {
                continue;
            }

            const sf::IntRect* rect = resMgr.getFrameRect(tileAtlasKey, config.frameName);
            if (!rect) continue;

            // 충돌 데이터 저장
            sf::FloatRect tileBounds({ i * tileSize.x, j * tileSize.y }, { tileSize.x, tileSize.y });
            m_collisionTiles.push_back({ tileBounds, config.type });

            // 6개의 정점 위치 구성 (삼각형 2개로 사각형 구성)
            sf::Vertex* quad = &m_vertices[index * 6];

            quad[0].position = sf::Vector2f(i * tileSize.x, j * tileSize.y);
            quad[1].position = sf::Vector2f((i + 1) * tileSize.x, j * tileSize.y);
            quad[2].position = sf::Vector2f(i * tileSize.x, (j + 1) * tileSize.y);

            quad[3].position = sf::Vector2f(i * tileSize.x, (j + 1) * tileSize.y);
            quad[4].position = sf::Vector2f((i + 1) * tileSize.x, j * tileSize.y);
            quad[5].position = sf::Vector2f((i + 1) * tileSize.x, (j + 1) * tileSize.y);

            // 텍스처 좌표 맵핑
            float tx = static_cast<float>(rect->position.x);
            float ty = static_cast<float>(rect->position.y);
            float tw = static_cast<float>(rect->size.x);
            float th = static_cast<float>(rect->size.y);

            quad[0].texCoords = sf::Vector2f(tx, ty);
            quad[1].texCoords = sf::Vector2f(tx + tw, ty);
            quad[2].texCoords = sf::Vector2f(tx, ty + th);

            quad[3].texCoords = sf::Vector2f(tx, ty + th);
            quad[4].texCoords = sf::Vector2f(tx + tw, ty);
            quad[5].texCoords = sf::Vector2f(tx + tw, ty + th);
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

    // 2. 타일맵 렌더링
    if (m_tileset) {
        states.texture = m_tileset;
        target.draw(m_vertices, states);
    }
}