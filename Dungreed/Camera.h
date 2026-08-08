#pragma once

#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/View.hpp>
#include <SFML/System/Vector2.hpp>
#include <algorithm>

/// 플레이어를 즉시 추적하는 2D 카메라입니다.
/// 확대된 뷰가 맵 경계를 벗어나지 않도록 중심 좌표를 제한합니다.
class Camera {
public:
    /// zoom은 1.0f가 원래 화면 크기이며, 값이 클수록 더 확대됩니다.
    Camera(const sf::Vector2u& windowSize, const sf::FloatRect& mapBounds,
        float zoom = 4.f);

    /// targetPosition을 바로 카메라 중심으로 적용합니다. 보간을 사용하지 않습니다.
    void update(const sf::Vector2f& targetPosition);

    /// 창 크기가 바뀌었을 때 호출합니다. 현재 확대 비율은 유지됩니다.
    void setWindowSize(const sf::Vector2u& windowSize);
    /// 확대 비율을 설정합니다. 1.0f 미만 값은 원래 화면 크기로 보정합니다.
    void setZoom(float zoom);
    /// 카메라가 움직일 수 있는 월드 영역을 바꿉니다.
    void setMapBounds(const sf::FloatRect& mapBounds);

    [[nodiscard]] float getZoom() const { return m_zoom; }
    [[nodiscard]] const sf::View& getView() const { return m_view; }

private:
    void refreshViewSize();
    sf::Vector2f clampCenter(const sf::Vector2f& desiredCenter) const;

    sf::View m_view;
    sf::Vector2u m_windowSize;
    sf::FloatRect m_mapBounds;
    float m_zoom = 1.f;
};