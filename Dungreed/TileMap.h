#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <optional>
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
};

// 게임 내 실제 물리 충돌에 쓰일 데이터
struct TileData {
    sf::FloatRect bounds;
    TileType type;
};

class TileMap : public sf::Drawable, public sf::Transformable {
public:
    TileMap() : m_vertices(sf::PrimitiveType::Triangles) {}
    ~TileMap() = default;

    // 타일맵 로드: 그리드 데이터 기반으로 VertexArray 구성
    bool load(const std::string& tileAtlasKey, const std::vector<TileConfig>& grid,
        unsigned int width, unsigned int height, sf::Vector2f tileSize);

    // 배경 이미지 설정
    void setBackground(const std::string& bgAtlasKey, const std::string& bgFrameName);

    // 충돌 처리를 위해 생성된 타일 물리 영역 반환
    inline const std::vector<TileData>& getCollisionTiles() const { return m_collisionTiles; }

protected:
    // sf::Drawable 인터페이스 구현 (window.draw(tileMap)을 위해)
    void draw(sf::RenderTarget& target, sf::RenderStates states) const override;

private:
    sf::VertexArray m_vertices;
    const sf::Texture* m_tileset = nullptr;
    std::vector<TileData> m_collisionTiles;

    std::optional<sf::Sprite> m_background;
};