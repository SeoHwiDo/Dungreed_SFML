#include "SceneTransition.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <utility>

namespace {
sf::String toSfUtf8String(std::string_view utf8Text) { return sf::String::fromUtf8(utf8Text.begin(), utf8Text.end()); }
} // namespace

bool SceneTransition::init(const sf::Font &font, const sf::Vector2u &size) {
    createCoverImage(size);
    m_loadingText.emplace(font, "", 26);
    m_loadingText->setFillColor(sf::Color::White);
    m_loadingText->setOutlineColor(sf::Color(16, 12, 28));
    m_loadingText->setOutlineThickness(2.f);
    return m_coverSprite.has_value();
}

bool SceneTransition::begin(GameScene destination, std::string destinationName, std::vector<LoadTask> loadTasks, std::function<void()> onLoaded) {
    if (isActive()) {
        return false;
    }

    m_destination = destination;
    m_destinationName = std::move(destinationName);
    m_loadTasks = std::move(loadTasks);
    m_onLoaded = std::move(onLoaded);
    m_nextTask = 0;
    m_phaseElapsed = 0.f;
    m_failed = false;
    m_errorMessage.clear();
    m_phase = Phase::FadeOut;
    return true;
}

void SceneTransition::update(float dt) {
    if (m_phase == Phase::Idle || m_failed) {
        return;
    }

    if (m_phase == Phase::FadeOut) {
        m_phaseElapsed += std::max(0.f, dt);
        if (m_phaseElapsed >= kFadeDuration) {
            m_phase = Phase::Loading;
            m_phaseElapsed = 0.f;
        }
        return;
    }

    if (m_phase == Phase::Loading) {
        if (m_nextTask < m_loadTasks.size()) {
            if (!m_loadTasks[m_nextTask]()) {
                m_failed = true;
                m_errorMessage = m_destinationName + u8"을(를) 불러오지 못했습니다.";
                return;
            }
            ++m_nextTask;
        }

        if (m_nextTask == m_loadTasks.size()) {
            if (m_onLoaded) {
                m_onLoaded();
            }
            m_phase = Phase::FadeIn;
            m_phaseElapsed = 0.f;
        }
        return;
    }

    m_phaseElapsed += std::max(0.f, dt);
    if (m_phaseElapsed >= kFadeDuration) {
        m_phase = Phase::Idle;
        m_phaseElapsed = 0.f;
        m_loadTasks.clear();
        m_onLoaded = nullptr;
    }
}

void SceneTransition::render(sf::RenderWindow &window) const {
    if (m_phase == Phase::Idle || !m_coverSprite) {
        return;
    }

    const std::uint8_t alpha = static_cast<std::uint8_t>(std::round(255.f * getOpacity()));
    sf::Sprite cover = *m_coverSprite;
    cover.setColor(sf::Color(255, 255, 255, alpha));
    window.draw(cover);

    if (!m_loadingText) {
        return;
    }

    sf::Text label = *m_loadingText;
    const std::string message = m_failed ? u8"불러오기에 실패했습니다" : m_destinationName + u8" 불러오는 중...";
    label.setString(toSfUtf8String(message));
    label.setFillColor(m_failed ? sf::Color(255, 188, 188, alpha) : sf::Color(255, 255, 255, alpha));
    const sf::FloatRect bounds = label.getLocalBounds();
    label.setOrigin(bounds.getCenter());
    const sf::Vector2u size = window.getSize();
    label.setPosition({size.x * 0.5f, size.y * 0.64f});
    window.draw(label);
}

bool SceneTransition::isActive() const { return m_phase != Phase::Idle; }

void SceneTransition::createCoverImage(const sf::Vector2u &size) {
    sf::Image image(size, sf::Color::Black);
    const float width = static_cast<float>(std::max(1u, size.x));
    const float height = static_cast<float>(std::max(1u, size.y));
    for (unsigned int y = 0; y < size.y; ++y) {
        for (unsigned int x = 0; x < size.x; ++x) {
            const float vertical = static_cast<float>(y) / height;
            const float horizontal = std::abs(static_cast<float>(x) / width - 0.5f) * 2.f;
            const float shade = std::clamp(1.f - horizontal * 0.55f - vertical * 0.22f, 0.f, 1.f);
            image.setPixel({x, y}, sf::Color(static_cast<std::uint8_t>(22.f + 28.f * shade), static_cast<std::uint8_t>(15.f + 18.f * shade), static_cast<std::uint8_t>(38.f + 42.f * shade)));
        }
    }

    m_coverTexture.emplace(image);
    m_coverTexture->setSmooth(true);
    m_coverSprite.emplace(*m_coverTexture);
}

float SceneTransition::getOpacity() const {
    if (m_failed || m_phase == Phase::Loading) {
        return 1.f;
    }
    const float progress = std::clamp(m_phaseElapsed / kFadeDuration, 0.f, 1.f);
    return m_phase == Phase::FadeOut ? progress : 1.f - progress;
}
