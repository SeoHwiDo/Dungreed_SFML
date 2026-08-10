#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <optional>
#include "Animator.h"
#include "ResourceManager.h"

// 타일의 물리적 특성 정의
enum class TileType {
    None,    // 빈 공간
    Solid,   // 완전한 바닥/벽 (상하좌우 모두 충돌)
    OneWay   // 공중 바닥 (위에서 아래로 떨어질 때만 충돌, 아래에서 위로는 통과)
};

// 각 타일 셀의 구성 정보
struct TileConfig {
    std::string frameName; // ResourceManager에 등록된 프레임 이름 (빈 문자열이면 None)
    TileType type = TileType::None;
    bool isBackground = false; // true면 렌더링만 하고 충돌을 만들지 않음
    int rotationQuarterTurns = 0; // 시계 방향 90도 단위 텍스처 회전
};

// 격자 한 칸에 맞추지 않고, 충돌 없이 맵을 꾸미는 스프라이트 설정입니다.
// position은 타일 단위의 월드 좌표이며 소수 값을 사용해 자유롭게 배치할 수 있습니다.
struct DecorativeTileConfig {
    std::string atlasKey;
    std::string frameName;
    std::string animationName;
    sf::Vector2f position{};
    sf::Vector2f offset{};
    sf::Vector2f scale{ 1.f, 1.f };
    float frameDuration = 0.15f;
    bool isLoop = true;
    // false면 벽·플랫폼보다 뒤, true면 그 앞에 그립니다. 어느 경우에도 충돌은 만들지 않습니다.
    bool drawAboveTiles = false;
};

// 게임 내 실제 물리 충돌에 쓰일 데이터
struct TileData {
    sf::FloatRect bounds;
    TileType type;
};

class TileMap : public sf::Drawable, public sf::Transformable {
public:
    /// 비어 있는 삼각형 버텍스 배열을 갖는 타일맵을 생성합니다. 실제 내용은 load로 구성합니다.
    TileMap()
        : m_vertices(sf::PrimitiveType::Triangles),
          m_backgroundVertices(sf::PrimitiveType::Triangles) {}
    ~TileMap() = default;

    /// 셀 그리드와 아틀라스로 렌더링 버텍스 및 물리 충돌 타일을 구성합니다.
    /// 셀 크기는 첫 유효 프레임의 sourceSize에서 자동으로 읽습니다.
    bool load(const std::string& tileAtlasKey, const std::vector<TileConfig>& grid,
        unsigned int width, unsigned int height,
        const std::vector<DecorativeTileConfig>& decorations = {});

    /// 장식 스프라이트의 애니메이션 프레임만 갱신합니다. 충돌 타일에는 영향을 주지 않습니다.
    void update(float dt) const;

    /// 타일맵 뒤에 그릴 단일 배경 스프라이트를 설정합니다. 프레임을 못 찾으면 배경을 비웁니다.
    void setBackground(const std::string& bgAtlasKey, const std::string& bgFrameName);

    /// Solid/OneWay 셀에서 생성한 충돌 영역 목록을 반환합니다.
    inline const std::vector<TileData>& getCollisionTiles() const { return m_collisionTiles; }
    /// 한 타일 셀의 월드 크기를 반환합니다.
    inline sf::Vector2f getTileSize() const { return m_tileSize; }
    /// 타일맵 전체의 픽셀/월드 크기를 반환합니다. 방 배치와 디버그 미리보기에 사용합니다.
    inline sf::Vector2f getPixelSize() const {
        return { m_tileSize.x * m_width, m_tileSize.y * m_height };
    }

protected:
    /// sf::Drawable 구현입니다. 배경 → 어두운 백타일 → 일반 타일 순으로 렌더링합니다.
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

private:
    struct AnimatedDecoration {
        sf::Sprite sprite;
        Animator animator;
        bool isAnimated = false;
    };

    /// JSON 장식 설정을 독립 스프라이트로 만들고, 렌더링 순서별 목록에 넣습니다.
    bool createDecorations(const std::string& defaultAtlasKey,
        const std::vector<DecorativeTileConfig>& decorations);

    sf::VertexArray m_backgroundVertices;
    sf::VertexArray m_vertices;
    const sf::Texture* m_tileset = nullptr;
    std::vector<TileData> m_collisionTiles;
    sf::Vector2f m_tileSize{ 0.f, 0.f };
    unsigned int m_width = 0;
    unsigned int m_height = 0;

    std::optional<sf::Sprite> m_background;
    mutable std::vector<AnimatedDecoration> m_backDecorations;
    mutable std::vector<AnimatedDecoration> m_frontDecorations;
};
