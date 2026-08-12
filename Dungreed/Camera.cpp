#include "Camera.h"

Camera::Camera(const sf::Vector2u &windowSize, const sf::FloatRect &mapBounds, float zoom) : m_windowSize(windowSize), m_mapBounds(mapBounds) {
    setZoom(zoom);
    update({m_mapBounds.position.x + m_mapBounds.size.x / 2.f, m_mapBounds.position.y + m_mapBounds.size.y / 2.f});
}

void Camera::update(const sf::Vector2f &targetPosition) { m_view.setCenter(clampCenter(targetPosition)); }

void Camera::setWindowSize(const sf::Vector2u &windowSize) {
    m_windowSize = windowSize;
    refreshViewSize();
}

void Camera::setZoom(float zoom) {
    m_zoom = std::max(zoom, 1.f);
    refreshViewSize();
}

void Camera::setMapBounds(const sf::FloatRect &mapBounds) {
    m_mapBounds = mapBounds;
    update(m_view.getCenter());
}

void Camera::refreshViewSize() {
    const float width = static_cast<float>(m_windowSize.x) / m_zoom;
    const float height = static_cast<float>(m_windowSize.y) / m_zoom;
    m_view.setSize({width, height});
}

sf::Vector2f Camera::clampCenter(const sf::Vector2f &desiredCenter) const {
    const sf::Vector2f halfView = m_view.getSize() / 2.f;
    const float left = m_mapBounds.position.x;
    const float top = m_mapBounds.position.y;
    const float right = left + m_mapBounds.size.x;
    const float bottom = top + m_mapBounds.size.y;

    // 뷰보다 작은 맵은 해당 축의 중앙을 사용합니다. 일반적인 큰 맵에서는
    // target을 그대로 따라가다가 가장자리에서만 멈춥니다.
    const float centerX = m_mapBounds.size.x <= m_view.getSize().x ? (left + right) / 2.f : std::clamp(desiredCenter.x, left + halfView.x, right - halfView.x);
    const float centerY = m_mapBounds.size.y <= m_view.getSize().y ? (top + bottom) / 2.f : std::clamp(desiredCenter.y, top + halfView.y, bottom - halfView.y);

    return {centerX, centerY};
}