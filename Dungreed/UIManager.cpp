#include "UIManager.h"

#include "Boss.h"
#include "LogManager.h"
#include "Player.h"
#include "ResourceManager.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <string_view>

namespace {
sf::String toSfUtf8String(std::string_view utf8Text) { return sf::String::fromUtf8(utf8Text.begin(), utf8Text.end()); }
} // namespace

bool UIManager::init(sf::RenderWindow &window) {
    auto &resourceManager = ResourceManager::getInstance();
    m_texture = resourceManager.getAtlasTexture("UI");
    if (!m_texture) {
        LogManager::getInstance().error("UIManager", "UI 아틀라스를 불러오지 못했습니다.");
        return false;
    }

    m_lifeBack.emplace(*m_texture);
    m_lifeBase.emplace(*m_texture);
    m_cursor.emplace(*m_texture);
    if (!setSpriteFrame(*m_lifeBack, "PlayerLifeBack.png") || !setSpriteFrame(*m_lifeBase, "PlayerLifeBase.png") || !setSpriteFrame(*m_cursor, "ShootingCursor2.png")) {
        LogManager::getInstance().error("UIManager", "던전 HUD 프레임을 불러오지 못했습니다.");
        return false;
    }

    if (!createScaledLifeBarTexture()) {
        LogManager::getInstance().error("UIManager", "확대 체력바 텍스처를 만들지 못했습니다.");
        return false;
    }
    if (!createDashCountFlashTexture()) {
        LogManager::getInstance().error("UIManager", "대시 충전 반짝임 텍스처를 만들지 못했습니다.");
        return false;
    }

    m_lifeWaves.clear();
    m_lifeWaves.reserve(7);
    for (int index = 0; index <= 6; ++index) {
        m_lifeWaves.emplace_back(*m_texture);
        if (!setSpriteFrame(m_lifeWaves.back(), "LifeWave" + std::to_string(index) + ".png")) {
            LogManager::getInstance().error("UIManager", "체력바 물결 프레임을 불러오지 못했습니다.");
            return false;
        }
    }

    m_dashSlots.clear();
    m_dashSlots.reserve(3);
    m_dashRightEnd.emplace(*m_texture);
    if (!setSpriteFrame(*m_dashRightEnd, "DashBaseRightEnd.png")) {
        LogManager::getInstance().error("UIManager", "대시 게이지 오른쪽 끝 프레임을 불러오지 못했습니다.");
        return false;
    }
    m_previousDashCharges = -1;
    for (int index = 0; index < 3; ++index) {
        const std::string baseFrameName = index == 0 ? "DashCountBase_0.png" : "DashBase.png";
        m_dashSlots.emplace_back(*m_texture, *m_dashCountFlashTexture);
        DashSlot &dashSlot = m_dashSlots.back();
        const std::optional<sf::Vector2f> basePivot = resourceManager.getFramePivot("UI", baseFrameName);
        if (!setSpriteFrame(dashSlot.base, baseFrameName) || !basePivot || !setSpriteFrame(dashSlot.count, "DashCount.png")) {
            LogManager::getInstance().error("UIManager", "대시 횟수 프레임을 불러오지 못했습니다.");
            return false;
        }
        dashSlot.count.setOrigin(dashSlot.count.getLocalBounds().getCenter());
        dashSlot.flash.setOrigin(dashSlot.flash.getLocalBounds().getCenter());
        dashSlot.basePivot = *basePivot;
    }

    m_lifeBack->setPosition(m_healthPosition);
    m_lifeBack->setScale({kUIScale, kUIScale});
    m_lifeBase->setPosition(m_healthPosition);
    m_lifeBase->setScale({kUIScale, kUIScale});
    m_lifeBar->setPosition(m_healthPosition + sf::Vector2f{kHealthBarOffsetX * kUIScale, kHealthBarOffsetY * kUIScale});
    for (sf::Sprite &wave : m_lifeWaves) {
        wave.setScale({kUIScale, kUIScale});
    }
    m_cursor->setOrigin({10.5f, 10.5f});
    m_cursor->setScale({kUIScale, kUIScale});

    const sf::Font *defaultFont = resourceManager.getDefaultFont();
    if (!defaultFont) {
        LogManager::getInstance().error("UIManager", "ResourceManager의 기본 게임 폰트를 찾을 수 없습니다.");
        return false;
    }
    m_playerHealthText.emplace(*defaultFont, "", 18);
    m_playerHealthText->setFillColor(sf::Color::White);
    m_playerHealthText->setOutlineColor(sf::Color(20, 16, 28));
    m_playerHealthText->setOutlineThickness(1.5f);
    m_bossNameText.emplace(*defaultFont, "", 24);
    m_bossNameText->setFillColor(sf::Color::White);
    m_bossNameText->setOutlineColor(sf::Color(20, 16, 28));
    m_bossNameText->setOutlineThickness(2.f);
    m_bossLifeFill.emplace(*m_texture);
    m_bossLifeBase.emplace(*m_texture);
    m_bossPortrait.emplace(*m_texture);
    const sf::IntRect *bossLifeBackFrame = resourceManager.getFrameRect("UI", "BossLifeBack.png");
    if (!bossLifeBackFrame || !setSpriteFrame(*m_bossLifeFill, "LifeBar.png") || !setSpriteFrame(*m_bossLifeBase, "BossLifeBase.png") || !setSpriteFrame(*m_bossPortrait, "BossSkellPortrait.png")) {
        LogManager::getInstance().error("UIManager", "보스 HUD 프레임을 불러오지 못했습니다.");
        return false;
    }
    m_bossLifeBackFrame = *bossLifeBackFrame;
    m_bossLifeBase->setScale({kBossHudScale, kBossHudScale});
    m_bossPortrait->setScale({kBossHudScale, kBossHudScale});

    updateHealth(1.f, 1.f, 0.f);
    updatePlayerHealthText(1.f, 1.f);
    updateDashCharges(3, 1.f, 0.f);
    window.setMouseCursorVisible(false);
    return true;
}

void UIManager::update(const Player &player, float dt, const sf::RenderWindow &window) {
    updateHealth(player.getTmpHp(), player.getMaxHp(), dt);
    updatePlayerHealthText(player.getTmpHp(), player.getMaxHp());
    updateDashCharges(player.getDashCharges(), player.getDashRechargeProgress(), dt);
    updateBossHud(window);
    if (m_cursor) {
        m_cursor->setPosition(sf::Vector2f(sf::Mouse::getPosition(window)));
    }
}

void UIManager::attachBoss(const Boss *boss) { m_activeBoss = boss; }

void UIManager::detachBoss(const Boss *boss) {
    if (m_activeBoss == boss) {
        m_activeBoss = nullptr;
        m_showBossHud = false;
    }
}

void UIManager::updateBossHud(const sf::RenderWindow &window) {
    const Boss *boss = m_activeBoss;
    m_showBossHud = boss && !boss->dead() && m_bossLifeFill && m_bossLifeBase && m_bossPortrait;
    if (!m_showBossHud) {
        return;
    }

    constexpr int kBossLifeFillStartX = 22;
    constexpr int kBossLifeFillWidth = 100;
    constexpr int kBossLifeFillStartY = 3;
    const float centerX = static_cast<float>(window.getSize().x) * 0.5f;
    const sf::Vector2f barPosition{centerX - (m_bossLifeBackFrame.size.x * kBossHudScale) * 0.5f, 58.f};
    const float healthRatio = boss->getMaxHp() > 0.f ? std::clamp(boss->getTmpHp() / boss->getMaxHp(), 0.f, 1.f) : 0.f;
    m_bossLifeFill->setPosition(barPosition + sf::Vector2f{kBossLifeFillStartX * kBossHudScale, kBossLifeFillStartY * kBossHudScale});
    // LifeBar는 1픽셀 폭의 채움 프레임입니다. 체력 비율만큼 X축만 확장합니다.
    m_bossLifeFill->setScale({kBossLifeFillWidth * healthRatio * kBossHudScale, kBossHudScale});
    m_bossLifeBase->setPosition(barPosition);
    m_bossPortrait->setPosition(barPosition + sf::Vector2f{3.f * kBossHudScale, 3.f * kBossHudScale});

    if (m_bossNameText) {
        m_bossNameText->setString(toSfUtf8String(boss->getDisplayName()));
        const sf::FloatRect bounds = m_bossNameText->getLocalBounds();
        m_bossNameText->setOrigin(bounds.getCenter());
        m_bossNameText->setPosition({centerX, 38.f});
    }
}

void UIManager::render(sf::RenderWindow &window) const {
    if (!m_lifeBack || !m_lifeBar || !m_lifeBase || !m_dashRightEnd || !m_cursor) {
        return;
    }

    window.draw(*m_lifeBack);
    window.draw(*m_lifeBar);
    if (m_showLifeWave && !m_lifeWaves.empty()) {
        window.draw(m_lifeWaves[m_waveIndex]);
    }
    window.draw(*m_lifeBase);
    if (m_playerHealthText) {
        window.draw(*m_playerHealthText);
    }

    sf::Transform dashTransform;
    dashTransform.translate(m_dashPosition).scale({kUIScale, kUIScale});
    const sf::RenderStates dashRenderStates{dashTransform};
    for (const DashSlot &dashSlot : m_dashSlots) {
        window.draw(dashSlot.base, dashRenderStates);
    }
    window.draw(*m_dashRightEnd, dashRenderStates);
    for (const DashSlot &dashSlot : m_dashSlots) {
        window.draw(dashSlot.count, dashRenderStates);
    }
    for (const DashSlot &dashSlot : m_dashSlots) {
        window.draw(dashSlot.flash, dashRenderStates);
    }
    if (m_showBossHud) {
        if (m_bossNameText) {
            window.draw(*m_bossNameText);
        }
        window.draw(*m_bossLifeFill);
        window.draw(*m_bossPortrait);
        window.draw(*m_bossLifeBase);
    }
    window.draw(*m_cursor);
}

bool UIManager::setSpriteFrame(sf::Sprite &sprite, const std::string &frameName) const {
    const sf::IntRect *frameRect = ResourceManager::getInstance().getFrameRect("UI", frameName);
    if (!m_texture || !frameRect) {
        return false;
    }

    sprite.setTexture(*m_texture);
    sprite.setTextureRect(*frameRect);
    return true;
}

bool UIManager::createScaledLifeBarTexture() {
    const sf::IntRect *lifeBarFrame = ResourceManager::getInstance().getFrameRect("UI", "LifeBar.png");
    if (!m_texture || !lifeBarFrame) {
        return false;
    }

    const unsigned int scale = static_cast<unsigned int>(kUIScale);
    const unsigned int width = static_cast<unsigned int>(kHealthBarWidth * kUIScale);
    const unsigned int height = static_cast<unsigned int>(lifeBarFrame->size.y) * scale;
    const sf::Image atlasImage = m_texture->copyToImage();
    sf::Image scaledLifeBar({width, height}, sf::Color::Transparent);

    for (unsigned int sourceY = 0; sourceY < static_cast<unsigned int>(lifeBarFrame->size.y); ++sourceY) {
        const sf::Color color = atlasImage.getPixel({static_cast<unsigned int>(lifeBarFrame->position.x), static_cast<unsigned int>(lifeBarFrame->position.y) + sourceY});
        for (unsigned int y = 0; y < scale; ++y) {
            for (unsigned int x = 0; x < width; ++x) {
                scaledLifeBar.setPixel({x, sourceY * scale + y}, color);
            }
        }
    }

    m_lifeBarTexture.emplace(scaledLifeBar);
    m_lifeBarTexture->setSmooth(false);
    m_lifeBar.emplace(*m_lifeBarTexture);
    return true;
}

bool UIManager::createDashCountFlashTexture() {
    const sf::IntRect *dashCountFrame = ResourceManager::getInstance().getFrameRect("UI", "DashCount.png");
    if (!m_texture || !dashCountFrame) {
        return false;
    }
    m_dashCountFrame = *dashCountFrame;

    const sf::Image atlasImage = m_texture->copyToImage();
    sf::Image flashImage({static_cast<unsigned int>(dashCountFrame->size.x), static_cast<unsigned int>(dashCountFrame->size.y)}, sf::Color::Transparent);
    for (int y = 0; y < dashCountFrame->size.y; ++y) {
        for (int x = 0; x < dashCountFrame->size.x; ++x) {
            const sf::Color sourceColor = atlasImage.getPixel({static_cast<unsigned int>(dashCountFrame->position.x + x), static_cast<unsigned int>(dashCountFrame->position.y + y)});
            flashImage.setPixel({static_cast<unsigned int>(x), static_cast<unsigned int>(y)}, sf::Color(255, 255, 255, sourceColor.a));
        }
    }

    m_dashCountFlashTexture.emplace(flashImage);
    m_dashCountFlashTexture->setSmooth(false);
    return true;
}

void UIManager::updateHealth(float currentHp, float maxHp, float dt) {
    if (!m_lifeBar || !m_lifeBarTexture) {
        return;
    }

    const float healthRatio = maxHp > 0.f ? std::clamp(currentHp / maxHp, 0.f, 1.f) : 0.f;
    const float healthWidth = std::round(kHealthBarWidth * healthRatio);
    const int scaledHealthWidth = static_cast<int>(healthWidth * kUIScale);
    m_lifeBar->setTextureRect({{0, 0}, {scaledHealthWidth, static_cast<int>(m_lifeBarTexture->getSize().y)}});

    if (healthRatio < m_previousHealthRatio) {
        m_waveElapsed = 0.f;
    } else {
        m_waveElapsed += dt;
    }
    m_previousHealthRatio = healthRatio;

    if (!m_lifeWaves.empty()) {
        m_waveIndex = static_cast<std::size_t>(m_waveElapsed / kWaveFrameDuration) % m_lifeWaves.size();
        const float waveX = m_healthPosition.x + kHealthBarOffsetX * kUIScale + healthWidth * kUIScale;
        m_lifeWaves[m_waveIndex].setPosition({waveX, m_healthPosition.y + kHealthBarOffsetY * kUIScale});
    }
    m_showLifeWave = healthRatio > 0.f && healthRatio < 1.f;
}

void UIManager::updatePlayerHealthText(float currentHp, float maxHp) {
    if (!m_playerHealthText) {
        return;
    }
    const int current = static_cast<int>(std::lround(std::max(0.f, currentHp)));
    const int maximum = static_cast<int>(std::lround(std::max(0.f, maxHp)));
    m_playerHealthText->setString(std::to_string(current) + " / " + std::to_string(maximum));
    const sf::FloatRect bounds = m_playerHealthText->getLocalBounds();
    m_playerHealthText->setOrigin(bounds.getCenter());
    m_playerHealthText->setPosition(m_healthPosition + sf::Vector2f{37.f * kUIScale, 8.f * kUIScale});
}

void UIManager::updateDashCharges(int charges, float rechargeProgress, float dt) {
    const int visibleCharges = std::clamp(charges, 0, static_cast<int>(m_dashSlots.size()));
    if (m_previousDashCharges >= 0 && visibleCharges > m_previousDashCharges) {
        for (int index = m_previousDashCharges; index < visibleCharges; ++index) {
            m_dashSlots[static_cast<std::size_t>(index)].flashTimer = kDashChargeFlashDuration;
        }
    }
    m_previousDashCharges = visibleCharges;

    float x = 0.f;
    for (std::size_t index = 0; index < m_dashSlots.size(); ++index) {
        DashSlot &dashSlot = m_dashSlots[index];
        sf::Sprite &base = dashSlot.base;
        base.setPosition({x, 0.f});

        sf::Sprite &count = dashSlot.count;
        const bool isCharged = index < static_cast<std::size_t>(visibleCharges);
        const bool isRecharging = !isCharged && index == static_cast<std::size_t>(visibleCharges) && visibleCharges < static_cast<int>(m_dashSlots.size());
        const sf::Vector2f countCenter = base.getPosition() + dashSlot.basePivot + sf::Vector2f{-0.5f, 0.f};

        count.setTextureRect(m_dashCountFrame);
        count.setOrigin({m_dashCountFrame.size.x / 2.f, m_dashCountFrame.size.y / 2.f});
        count.setPosition(countCenter);
        count.setColor(isCharged ? sf::Color::White : sf::Color(255, 255, 255, 0));

        if (isRecharging) {
            const float progress = std::clamp(rechargeProgress, 0.f, 1.f);
            const int fillWidth = std::clamp(static_cast<int>(std::ceil(m_dashCountFrame.size.x * progress)), 0, m_dashCountFrame.size.x);
            if (fillWidth > 0) {
                count.setTextureRect({m_dashCountFrame.position, {fillWidth, m_dashCountFrame.size.y}});
                count.setOrigin({0.f, m_dashCountFrame.size.y / 2.f});
                count.setPosition({countCenter.x - m_dashCountFrame.size.x / 2.f, countCenter.y});
                count.setColor(sf::Color(255, 255, 255, 128));
            }
        }

        sf::Sprite &flash = dashSlot.flash;
        flash.setPosition(count.getPosition());
        const float flashRatio = dashSlot.flashTimer / kDashChargeFlashDuration;
        flash.setColor(isCharged && flashRatio > 0.f ? sf::Color(255, 255, 255, static_cast<std::uint8_t>(std::round(255.f * flashRatio))) : sf::Color(255, 255, 255, 0));
        if (!isCharged) {
            dashSlot.flashTimer = 0.f;
        }
        x += base.getLocalBounds().size.x;
    }

    for (DashSlot &dashSlot : m_dashSlots) {
        dashSlot.flashTimer = std::max(0.f, dashSlot.flashTimer - dt);
    }
    m_dashRightEnd->setPosition({x, 0.f});
}
