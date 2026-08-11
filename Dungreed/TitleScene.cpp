#include "TitleScene.h"

#include "ResourceManager.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>

namespace {
sf::String toSfUtf8String(std::string_view utf8Text) {
    return sf::String::fromUtf8(utf8Text.begin(), utf8Text.end());
}

void drawTitleSky(sf::RenderWindow& window, const sf::Texture& texture) {
    const sf::Vector2u textureSize = texture.getSize();
    const sf::Vector2u windowSize = window.getSize();
    if (textureSize.x == 0 || textureSize.y == 0) {
        return;
    }

    sf::Sprite sky(texture);
    sky.setScale({
        static_cast<float>(windowSize.x) / static_cast<float>(textureSize.x),
        static_cast<float>(windowSize.y) / static_cast<float>(textureSize.y)
    });
    window.draw(sky);
}

void drawRepeatingClouds(sf::RenderWindow& window, const sf::Texture& texture,
    float offset, float opacity) {
    const sf::Vector2u textureSize = texture.getSize();
    const sf::Vector2u windowSize = window.getSize();
    if (textureSize.x == 0 || textureSize.y == 0) {
        return;
    }

    const float scale = static_cast<float>(windowSize.y) / static_cast<float>(textureSize.y);
    const float scaledWidth = static_cast<float>(textureSize.x) * scale;
    const float wrappedOffset = std::fmod(offset, scaledWidth);
    for (float x = -wrappedOffset; x < static_cast<float>(windowSize.x); x += scaledWidth) {
        sf::Sprite cloud(texture);
        cloud.setScale({ scale, scale });
        cloud.setPosition({ x, 0.f });
        cloud.setColor(sf::Color(255, 255, 255,
            static_cast<std::uint8_t>(std::clamp(opacity, 0.f, 255.f))));
        window.draw(cloud);
    }
}

void drawMidCloud(sf::RenderWindow& window, const sf::Texture& texture,
    float x, float y, float scale) {
    sf::Sprite cloud(texture);
    cloud.setScale({ scale, scale });
    cloud.setPosition({ x, y });
    cloud.setColor(sf::Color(255, 255, 255, 190));
    window.draw(cloud);
}
}

bool TitleScene::enter() {
    auto& resources = ResourceManager::getInstance();
    if (!resources.loadTitleResources()) {
        return false;
    }

    const sf::Font* font = resources.getDefaultFont();
    m_logoTexture = resources.getAtlasTexture("TitleLogo");
    m_skyTexture = resources.getAtlasTexture("TitleSky");
    m_backCloudTexture = resources.getAtlasTexture("TitleBackCloud");
    m_midCloud0Texture = resources.getAtlasTexture("TitleMidCloud0");
    m_midCloud1Texture = resources.getAtlasTexture("TitleMidCloud1");
    m_frontCloudTexture = resources.getAtlasTexture("TitleFrontCloud");
    if (!font || !m_logoTexture || !m_skyTexture || !m_backCloudTexture ||
        !m_midCloud0Texture || !m_midCloud1Texture || !m_frontCloudTexture) {
        return false;
    }

    m_startText.emplace(*font, toSfUtf8String(u8"Enter 키를 눌러 게임 시작"), 26);
    m_startText->setFillColor(sf::Color(32, 68, 102));
    m_startText->setOutlineColor(sf::Color(238, 248, 255));
    m_startText->setOutlineThickness(1.f);
    m_elapsed = 0.f;
    return true;
}

void TitleScene::update(float dt) {
    m_elapsed += std::max(0.f, dt);
}

void TitleScene::render() {
    if (!m_startText || !m_logoTexture || !m_skyTexture || !m_backCloudTexture ||
        !m_midCloud0Texture || !m_midCloud1Texture || !m_frontCloudTexture) {
        return;
    }

    m_window.setView(m_window.getDefaultView());
    const sf::Vector2u windowSize = m_window.getSize();
    const float sceneScale = std::min(
        static_cast<float>(windowSize.x) / 1280.f,
        static_cast<float>(windowSize.y) / 720.f);
    const float screenWidth = static_cast<float>(windowSize.x);
    const float screenHeight = static_cast<float>(windowSize.y);

    drawTitleSky(m_window, *m_skyTexture);
    drawRepeatingClouds(m_window, *m_backCloudTexture, m_elapsed * 9.f, 150.f);
    drawMidCloud(m_window, *m_midCloud0Texture,
        std::fmod(m_elapsed * 18.f + screenWidth * 0.12f, screenWidth + 260.f) - 130.f,
        screenHeight * 0.29f, sceneScale * 4.f);
    drawMidCloud(m_window, *m_midCloud1Texture,
        std::fmod(m_elapsed * 13.f + screenWidth * 0.68f, screenWidth + 360.f) - 180.f,
        screenHeight * 0.52f, sceneScale * 3.5f);
    drawRepeatingClouds(m_window, *m_frontCloudTexture, m_elapsed * 15.f, 155.f);

    sf::Sprite logo(*m_logoTexture);
    const sf::Vector2u logoSize = m_logoTexture->getSize();
    logo.setOrigin({ logoSize.x * 0.5f, logoSize.y * 0.5f });
    logo.setScale({ sceneScale * 3.f, sceneScale * 3.f });
    logo.setPosition({ screenWidth * 0.5f,
        screenHeight * 0.37f + std::sin(m_elapsed * 1.5f) * 5.f * sceneScale });
    m_window.draw(logo);

    const float pulse = 0.68f + 0.32f * (std::sin(m_elapsed * 3.f) + 1.f) * 0.5f;
    m_startText->setFillColor(sf::Color(32, 68, 102,
        static_cast<std::uint8_t>(255.f * pulse)));
    placeStartText();
    m_window.draw(*m_startText);
}

void TitleScene::placeStartText() {
    if (!m_startText) {
        return;
    }
    const sf::FloatRect bounds = m_startText->getLocalBounds();
    m_startText->setOrigin(bounds.getCenter());
    m_startText->setPosition({
        static_cast<float>(m_window.getSize().x) * 0.5f,
        static_cast<float>(m_window.getSize().y) * 0.68f
    });
}
