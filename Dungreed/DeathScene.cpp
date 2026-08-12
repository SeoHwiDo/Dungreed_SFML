#include "DeathScene.h"

#include "ResourceManager.h"

#include <algorithm>
#include <cstdint>
#include <string_view>
#include <utility>

namespace {
sf::String toSfUtf8String(std::string_view utf8Text) { return sf::String::fromUtf8(utf8Text.begin(), utf8Text.end()); }

std::uint8_t getLuminance(sf::Color color) { return static_cast<std::uint8_t>(0.299f * color.r + 0.587f * color.g + 0.114f * color.b); }

sf::Color colorizeResult(sf::Color color, ResultType resultType) {
    const std::uint8_t luminance = getLuminance(color);
    if (resultType == ResultType::Failure) {
        return {luminance, luminance, luminance, color.a};
    }

    return {luminance, static_cast<std::uint8_t>(std::min(255, luminance * 85 / 100)), static_cast<std::uint8_t>(std::min(255, luminance * 28 / 100)), color.a};
}
} // namespace

bool DeathScene::init(const sf::Font &font) {
    m_messageText.emplace(font, toSfUtf8String(u8"사망하였습니다"), 25);
    m_helpText.emplace(font, toSfUtf8String(u8"Enter키를 누르면 마을로 돌아갑니다"), 20);

    for (sf::Text *text : {&*m_messageText, &*m_helpText}) {
        text->setFillColor(sf::Color::White);
        text->setOutlineColor(sf::Color(20, 20, 20));
        text->setOutlineThickness(2.f);
    }
    return true;
}

bool DeathScene::enter(ResultType resultType) {
    auto &resources = ResourceManager::getInstance();
    const sf::Texture *uiTexture = resources.getAtlasTexture("UI");
    const char *frameName = resultType == ResultType::Failure ? "ResultFail.png" : "ResultSuccess.png";
    const sf::IntRect *frame = resources.getFrameRect("UI", frameName);
    if (!uiTexture || !frame || !m_messageText) {
        return false;
    }

    m_resultType = resultType;
    m_resultSprite.emplace(*uiTexture);
    m_resultSprite->setTextureRect(*frame);
    m_resultSprite->setScale({2.f, 2.f});
    m_messageText->setString(toSfUtf8String(resultType == ResultType::Failure ? u8"사망하였습니다" : u8"보스를 처치했습니다"));
    m_active = true;
    m_captureRequested = true;
    return true;
}

void DeathScene::leave() {
    m_active = false;
    m_captureRequested = false;
    m_snapshotSprite.reset();
    m_resultSprite.reset();
}

bool DeathScene::handleEvent(const sf::Event &event) const {
    const auto *key = event.getIf<sf::Event::KeyPressed>();
    return key && key->code == sf::Keyboard::Key::Enter;
}

void DeathScene::captureCurrentScreen() {
    if (!m_active || !m_captureRequested || !m_snapshotTexture.resize(m_window.getSize())) {
        return;
    }

    m_snapshotTexture.update(m_window);
    sf::Image image = m_snapshotTexture.copyToImage();
    const sf::Vector2u size = image.getSize();
    for (unsigned int y = 0; y < size.y; ++y) {
        for (unsigned int x = 0; x < size.x; ++x) {
            image.setPixel({x, y}, colorizeResult(image.getPixel({x, y}), m_resultType));
        }
    }

    if (!m_snapshotTexture.loadFromImage(image)) {
        return;
    }
    m_snapshotSprite.emplace(m_snapshotTexture);
    m_captureRequested = false;
}

void DeathScene::render() {
    if (!m_active || !m_snapshotSprite || !m_resultSprite || !m_messageText || !m_helpText) {
        return;
    }

    m_window.setView(m_window.getDefaultView());
    m_window.draw(*m_snapshotSprite);

    sf::RectangleShape dim({static_cast<float>(m_window.getSize().x), static_cast<float>(m_window.getSize().y)});
    dim.setFillColor(m_resultType == ResultType::Failure ? sf::Color(0, 0, 0, 85) : sf::Color(90, 55, 0, 55));
    m_window.draw(dim);

    placeTexts();
    m_window.draw(*m_resultSprite);
    m_window.draw(*m_messageText);
    m_window.draw(*m_helpText);
}

void DeathScene::placeTexts() {
    const float centerX = static_cast<float>(m_window.getSize().x) * 0.5f;
    m_resultSprite->setOrigin(m_resultSprite->getLocalBounds().getCenter());
    m_resultSprite->setPosition({centerX, 120.f});
    for (const auto &[text, y] : {std::pair{&*m_messageText, 185.f}, std::pair{&*m_helpText, 235.f}}) {
        text->setOrigin(text->getLocalBounds().getCenter());
        text->setPosition({centerX, y});
    }
}
