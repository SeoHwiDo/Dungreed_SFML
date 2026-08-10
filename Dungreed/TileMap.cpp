#include "TileMap.h"
#include <array>
#include <iostream>

/// 첫 유효 타일의 원본 크기를 셀 크기로 삼아, 전경/백타일 버텍스와 충돌 목록을 한 번에 재생성합니다.
bool TileMap::load(const std::string& tileAtlasKey, const std::vector<TileConfig>& grid,
    unsigned int width, unsigned int height,
    const std::vector<DecorativeTileConfig>& decorations) {
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
    for (std::size_t index = 0; index < m_vertices.getVertexCount(); ++index) {
        m_vertices[index].color = sf::Color::Transparent;
        m_backgroundVertices[index].color = sf::Color::Transparent;
    }
    m_collisionTiles.clear();

    for (unsigned int i = 0; i < width; ++i) {
        for (unsigned int j = 0; j < height; ++j) {
            unsigned int index = i + j * width;
            const TileConfig& config = grid[index];

            // 한 셀을 두 삼각형으로 만들고, 회전값에 맞게 텍스처 좌표만 재배치합니다.
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
    return createDecorations(tileAtlasKey, decorations);
}

/// 장식 타일은 일반 격자 버텍스와 달리 각자 다른 크기·아틀라스·애니메이션을 가질 수 있습니다.
/// 물리 TileData를 생성하지 않으므로 캐릭터와 충돌하거나 이동을 막지 않습니다.
bool TileMap::createDecorations(const std::string& defaultAtlasKey,
    const std::vector<DecorativeTileConfig>& decorations) {
    m_backDecorations.clear();
    m_frontDecorations.clear();

    auto& resMgr = ResourceManager::getInstance();
    for (const DecorativeTileConfig& config : decorations) {
        const std::string& atlasKey = config.atlasKey.empty()
            ? defaultAtlasKey
            : config.atlasKey;
        const sf::Texture* texture = resMgr.getAtlasTexture(atlasKey);
        if (!texture) {
            std::cerr << "[TileMap] 장식 타일 아틀라스를 찾을 수 없습니다: "
                << atlasKey << std::endl;
            return false;
        }

        AnimatedDecoration decoration{ sf::Sprite(*texture) };
        const bool isAnimated = !config.animationName.empty();
        const std::string& initialFrame = isAnimated
            ? config.animationName
            : config.frameName;
        if (initialFrame.empty()) {
            std::cerr << "[TileMap] 장식 타일에는 frame 또는 animation이 필요합니다." << std::endl;
            return false;
        }

        if (isAnimated) {
            const std::vector<sf::IntRect>* frames =
                resMgr.getAnimationFrames(atlasKey, config.animationName);
            if (!frames || frames->empty()) {
                std::cerr << "[TileMap] 장식 애니메이션을 찾을 수 없습니다: "
                    << config.animationName << std::endl;
                return false;
            }
            decoration.sprite.setTextureRect(frames->front());
            decoration.animator.addAnimation(config.animationName,
                AnimationClip(frames, config.frameDuration, config.isLoop));
            decoration.animator.play(config.animationName);
            decoration.isAnimated = true;
        } else {
            const sf::IntRect* frame = resMgr.getFrameRect(atlasKey, config.frameName);
            if (!frame) {
                std::cerr << "[TileMap] 장식 프레임을 찾을 수 없습니다: "
                    << config.frameName << std::endl;
                return false;
            }
            decoration.sprite.setTextureRect(*frame);
        }

        const std::optional<sf::Vector2f> pivot = isAnimated
            ? resMgr.getAnimationFirstFramePivot(atlasKey, config.animationName)
            : resMgr.getFramePivot(atlasKey, config.frameName);
        if (pivot) {
            decoration.sprite.setOrigin(*pivot);
        }
        decoration.sprite.setPosition({
            config.position.x * m_tileSize.x + config.offset.x,
            config.position.y * m_tileSize.y + config.offset.y
        });
        decoration.sprite.setScale(config.scale);

        std::vector<AnimatedDecoration>& layer = config.drawAboveTiles
            ? m_frontDecorations
            : m_backDecorations;
        layer.push_back(std::move(decoration));
    }
    return true;
}

void TileMap::update(float dt) const {
    const auto updateLayer = [dt](std::vector<AnimatedDecoration>& decorations) {
        for (AnimatedDecoration& decoration : decorations) {
            if (decoration.isAnimated) {
                decoration.animator.update(dt, decoration.sprite);
            }
        }
    };
    updateLayer(m_backDecorations);
    updateLayer(m_frontDecorations);
}

/// 단일 배경 프레임을 선택해 타일 버텍스보다 먼저 그릴 스프라이트로 보관합니다.
void TileMap::setBackground(const std::string& bgAtlasKey, const std::string& bgFrameName) {
    auto& resMgr = ResourceManager::getInstance();
    const sf::Texture* tex = resMgr.getAtlasTexture(bgAtlasKey);
    const sf::IntRect* rect = resMgr.getFrameRect(bgAtlasKey, bgFrameName);

    if (tex && rect) {
        m_background.emplace(*tex);
        m_background->setTextureRect(*rect);
    }
}

/// 타일맵 자체의 변환을 적용한 뒤, 배경 이미지·어두운 백타일·충돌 타일 순으로 그립니다.
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

        for (const AnimatedDecoration& decoration : m_backDecorations) {
            target.draw(decoration.sprite, states);
        }

        // 3. 상호작용 타일 렌더링
        target.draw(m_vertices, states);

        for (const AnimatedDecoration& decoration : m_frontDecorations) {
            target.draw(decoration.sprite, states);
        }
    }
}
