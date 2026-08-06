#include "Collision.h"
#include "Actor.h"
#include "TileMap.h"
#include<optional>
std::optional<sf::Vector2f> Collision::checkHit(const sf::FloatRect& attackBox) const {
    auto intersection = m_hitbox.findIntersection(attackBox);

    if (intersection.has_value()) {
        sf::FloatRect overlap = intersection.value();

        // 겹친 영역의 정중앙 좌표를 계산하여 반환
        return sf::Vector2f{
            overlap.position.x + (overlap.size.x / 2.f),
            overlap.position.y + (overlap.size.y / 2.f)
        };
    }

    return std::nullopt; // 충돌하지 않음
}

void Collision::resolveMapCollision(Actor& actor, const TileMap& map) {
    sf::FloatRect bounds = actor.getGlobalBounds();
    auto& movement = actor.getMovement();
    bool wasGrounded = false;

    for (const auto& tile : map.getCollisionTiles()) {
        if (tile.type == TileType::None) continue;

        // SFML 3.1.0 기준 반환값 optional 
        auto intersection = bounds.findIntersection(tile.bounds);
        if (intersection.has_value()) {
            sf::FloatRect overlap = intersection.value();

            // 겹친 영역 중 더 작은 쪽을 밀어내는 방향으로 결정 (AABB 충돌 해결)
            if (overlap.size.x < overlap.size.y) {
                // 수평(좌우) 충돌 해결
                if (bounds.position.x < tile.bounds.position.x) {
                    actor.move(-overlap.size.x, 0.f);
                } else {
                    actor.move(overlap.size.x, 0.f);
                }
                movement.velocity.x = 0.f;
            } else {
                // 수직(상하) 충돌 해결
                if (bounds.position.y < tile.bounds.position.y) {
                    // 바닥(위에서 아래로 타일과 충돌)
                    actor.move(0.f, -overlap.size.y);
                    movement.velocity.y = 0.f;
                    wasGrounded = true;
                } else {
                    // 천장(아래에서 위로 타일과 충돌) - OneWay 타일이면 무시
                    if (tile.type == TileType::Solid) {
                        actor.move(0.f, overlap.size.y);
                        movement.velocity.y = 0.f;
                    }
                }
            }
            // 다음 타일과의 정밀한 비교를 위해 위치가 갱신된 바운드 다시 가져오기
            bounds = actor.getGlobalBounds();
        }
    }
    movement.isGrounded = wasGrounded;
}