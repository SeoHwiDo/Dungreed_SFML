#include "UIManager.h"

#include "Player.h"
#include "ResourceManager.h"

#include <algorithm>
#include <cmath>
#include <iostream>

bool UIManager::init(sf::RenderWindow& window) {
    auto& resourceManager = ResourceManager::getInstance();
    if (!resourceManager.loadAtlas("UI", std::string(kUiAtlasJsonPath), std::string(kUiAtlasPath))) {
        std::cerr << "[UIManager] UI 아틀라스를 불러오지 못했습니다.\n";
        return false;
    }
    m_texture = resourceManager.getAtlasTexture("UI");
    if (!m_texture) {
        std::cerr << "[UIManager] UI 아틀라스를 불러오지 못했습니다.\n";
        return false;
    }

    m_lifeBack.emplace(*m_texture);
    m_lifeBase.emplace(*m_texture);
    m_cursor.emplace(*m_texture);
    if (!setSpriteFrame(*m_lifeBack, "PlayerLifeBack.png") ||
        !setSpriteFrame(*m_lifeBase, "PlayerLifeBase.png") ||
        !setSpriteFrame(*m_cursor, "ShootingCursor2.png")) {
        std::cerr << "[UIManager] 던전 HUD 프레임을 불러오지 못했습니다.\n";
        return false;
    }

    if (!createScaledLifeBarTexture()) {
        std::cerr << "[UIManager] 확대 체력바 텍스처를 만들지 못했습니다.\n";
        return false;
    }

    m_lifeWaves.clear();
    m_lifeWaves.reserve(7);
    for (int index = 0; index <= 6; ++index) {
        m_lifeWaves.emplace_back(*m_texture);
        if (!setSpriteFrame(m_lifeWaves.back(), "LifeWave" + std::to_string(index) + ".png")) {
            std::cerr << "[UIManager] 체력바 물결 프레임을 불러오지 못했습니다.\n";
            return false;
        }
    }

    m_dashBases.clear();
    m_dashCounts.clear();
    m_dashBases.reserve(3);
    m_dashCounts.reserve(3);
    for (int index = 0; index < 3; ++index) {
        m_dashBases.emplace_back(*m_texture);
        m_dashCounts.emplace_back(*m_texture);
        if (!setSpriteFrame(m_dashBases.back(), "DashCountBase_" + std::to_string(index) + ".png") ||
            !setSpriteFrame(m_dashCounts.back(), "DashCount.png")) {
            std::cerr << "[UIManager] 대시 횟수 프레임을 불러오지 못했습니다.\n";
            return false;
        }
        m_dashBases.back().setScale({ kUIScale, kUIScale });
        m_dashCounts.back().setScale({ kUIScale, kUIScale });
    }

    m_lifeBack->setPosition(m_healthPosition);
    m_lifeBack->setScale({ kUIScale, kUIScale });
    m_lifeBase->setPosition(m_healthPosition);
    m_lifeBase->setScale({ kUIScale, kUIScale });
    m_lifeBar->setPosition(m_healthPosition + sf::Vector2f{ kHealthBarOffsetX * kUIScale, kHealthBarOffsetY * kUIScale });
    for (sf::Sprite& wave : m_lifeWaves) {
        wave.setScale({ kUIScale, kUIScale });
    }
    m_cursor->setOrigin({ 10.5f, 10.5f });
    m_cursor->setScale({ kUIScale, kUIScale });
    updateHealth(1.f, 1.f, 0.f);
    updateDashCharges(3);
    window.setMouseCursorVisible(false);
    return true;
}

void UIManager::update(const Player& player, float dt, const sf::RenderWindow& window) {
    updateHealth(player.getTmpHp(), player.getMaxHp(), dt);
    updateDashCharges(player.getDashCharges());
    if (m_cursor) {
        m_cursor->setPosition(sf::Vector2f(sf::Mouse::getPosition(window)));
    }
}

void UIManager::render(sf::RenderWindow& window) const {
    if (!m_lifeBack || !m_lifeBar || !m_lifeBase || !m_cursor) {
        return;
    }

    window.draw(*m_lifeBack);
    window.draw(*m_lifeBar);
    if (m_showLifeWave && !m_lifeWaves.empty()) {
        window.draw(m_lifeWaves[m_waveIndex]);
    }
    window.draw(*m_lifeBase);

    for (const sf::Sprite& base : m_dashBases) {
        window.draw(base);
    }
    for (const sf::Sprite& count : m_dashCounts) {
        window.draw(count);
    }
    window.draw(*m_cursor);
}

bool UIManager::setSpriteFrame(sf::Sprite& sprite, const std::string& frameName) const {
    const sf::IntRect* frameRect = ResourceManager::getInstance().getFrameRect("UI", frameName);
    if (!m_texture || !frameRect) {
        return false;
    }

    sprite.setTexture(*m_texture);
    sprite.setTextureRect(*frameRect);
    return true;
}

bool UIManager::createScaledLifeBarTexture() {
    const sf::IntRect* lifeBarFrame =
        ResourceManager::getInstance().getFrameRect("UI", "LifeBar.png");
    if (!m_texture || !lifeBarFrame) {
        return false;
    }

    const unsigned int scale = static_cast<unsigned int>(kUIScale);
    const unsigned int width = static_cast<unsigned int>(kHealthBarWidth * kUIScale);
    const unsigned int height = static_cast<unsigned int>(lifeBarFrame->size.y) * scale;
    const sf::Image atlasImage = m_texture->copyToImage();
    sf::Image scaledLifeBar({ width, height }, sf::Color::Transparent);

    for (unsigned int sourceY = 0; sourceY < static_cast<unsigned int>(lifeBarFrame->size.y); ++sourceY) {
        const sf::Color color = atlasImage.getPixel({
            static_cast<unsigned int>(lifeBarFrame->position.x),
            static_cast<unsigned int>(lifeBarFrame->position.y) + sourceY
        });
        for (unsigned int y = 0; y < scale; ++y) {
            for (unsigned int x = 0; x < width; ++x) {
                scaledLifeBar.setPixel({ x, sourceY * scale + y }, color);
            }
        }
    }

    m_lifeBarTexture.emplace(scaledLifeBar);
    m_lifeBarTexture->setSmooth(false);
    m_lifeBar.emplace(*m_lifeBarTexture);
    return true;
}
void UIManager::updateHealth(float currentHp, float maxHp, float dt) {
    if (!m_lifeBar || !m_lifeBarTexture) {
        return;
    }

    const float healthRatio = maxHp > 0.f
        ? std::clamp(currentHp / maxHp, 0.f, 1.f)
        : 0.f;
    const float healthWidth = std::round(kHealthBarWidth * healthRatio);
    const int scaledHealthWidth = static_cast<int>(healthWidth * kUIScale);
    m_lifeBar->setTextureRect({
        { 0, 0 },
        { scaledHealthWidth, static_cast<int>(m_lifeBarTexture->getSize().y) }
    });

    if (healthRatio < m_previousHealthRatio) {
        m_waveElapsed = 0.f;
    } else {
        m_waveElapsed += dt;
    }
    m_previousHealthRatio = healthRatio;

    if (!m_lifeWaves.empty()) {
        m_waveIndex = static_cast<std::size_t>(m_waveElapsed / kWaveFrameDuration) % m_lifeWaves.size();
        const float waveX = m_healthPosition.x + kHealthBarOffsetX * kUIScale +
            healthWidth * kUIScale;
        m_lifeWaves[m_waveIndex].setPosition({
            waveX, m_healthPosition.y + kHealthBarOffsetY * kUIScale
        });
    }
    m_showLifeWave = healthRatio > 0.f && healthRatio < 1.f;
}

void UIManager::updateDashCharges(int charges) {
    float x = m_dashPosition.x;
    for (std::size_t index = 0; index < m_dashBases.size(); ++index) {
        sf::Sprite& base = m_dashBases[index];
        base.setPosition({ x, m_dashPosition.y });

        sf::Sprite& count = m_dashCounts[index];
        const float countOffsetX = index == 1 ? 0.f : kUIScale;
        count.setPosition({
            x + countOffsetX, m_dashPosition.y + 2.f * kUIScale
        });
        count.setColor(index < static_cast<std::size_t>(std::max(0, charges))
            ? sf::Color::White
            : sf::Color(255, 255, 255, 0));
        x += base.getLocalBounds().size.x * kUIScale;
    }
}
