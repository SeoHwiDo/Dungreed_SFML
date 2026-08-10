#include "RewardChestManager.h"

#include <algorithm>
#include <cmath>
#include <iostream>

#include "Player.h"
#include "EffectManager.h"
#include "ObjectPoolingManager.h"
#include "ResourceManager.h"
#include "Room.h"
#include "TileMap.h"

namespace {
constexpr std::array<const char*, 4> kFairyAtlasKeys{
    "FairyS", "FairyM", "FairyL", "FairyXL"
};
constexpr std::array<const char*, 4> kFairyFrameNames{
    "FairyS", "FairyM", "FairyL", "FairyXL"
};

std::size_t getFairyIndex(FairyRewardSize size) {
    return static_cast<std::size_t>(size);
}
}

bool RewardChestManager::init() {
    auto& resourceManager = ResourceManager::getInstance();
    const auto createSprite = [&resourceManager](const char* atlasKey, const char* frameName,
        bool bottomCenterOrigin) -> std::optional<sf::Sprite> {
        const sf::Texture* texture = resourceManager.getAtlasTexture(atlasKey);
        const sf::IntRect* frame = resourceManager.getFrameRect(atlasKey, frameName);
        if (!texture || !frame) {
            return std::nullopt;
        }

        sf::Sprite sprite(*texture);
        sprite.setTextureRect(*frame);
        sprite.setOrigin(bottomCenterOrigin
            ? sf::Vector2f{ frame->size.x * 0.5f, static_cast<float>(frame->size.y) }
            : sf::Vector2f{ frame->size.x * 0.5f, frame->size.y * 0.5f });
        return sprite;
    };

    m_closedTreasureSprite = createSprite("TileMap", "BasicTresureClosed.png", true);
    m_openedTreasureSprite = createSprite("TileMap", "BasicTresureOpened.png", true);
    const sf::Texture* monsterTexture = resourceManager.getAtlasTexture("Monster");
    for (std::size_t index = 0; index < m_fairySprites.size(); ++index) {
        const std::vector<sf::IntRect>* frames = resourceManager.getAnimationFrames(
            "Monster", kFairyAtlasKeys[index]);
        if (!monsterTexture || !frames || frames->empty()) {
            continue;
        }

        FairyVisual fairy{ sf::Sprite(*monsterTexture), Animator{} };
        fairy.sprite.setTextureRect(frames->front());
        fairy.sprite.setOrigin(resourceManager.getAnimationFirstFramePivot(
            "Monster", kFairyAtlasKeys[index]).value_or(sf::Vector2f{
                frames->front().size.x * 0.5f, frames->front().size.y * 0.5f }));
        fairy.animator.addAnimation(kFairyFrameNames[index],
            AnimationClip(frames, 0.1f, true));
        fairy.animator.play(kFairyFrameNames[index]);
        m_fairySprites[index] = std::move(fairy);
    }

    if (!m_closedTreasureSprite || !m_openedTreasureSprite ||
        std::any_of(m_fairySprites.begin(), m_fairySprites.end(),
            [](const auto& fairy) { return !fairy.has_value(); })) {
        std::cerr << "[RewardChest] 보물상자 또는 요정 스프라이트를 찾을 수 없습니다.\n";
        return false;
    }
    return true;
}

void RewardChestManager::update(float dt, Room& room, const TileMap& tileMap,
    Player& player, EffectManager& effectManager, ObjectPoolingManager& objectPool) {
    // 처음 방에 들어왔을 때는 전투가 끝나지 않아 Hidden 상태가 될 수 있습니다.
    // 같은 방이라도 클리어 조건이 새로 충족되면 상자를 다시 활성화합니다.
    if (&room != m_room || (m_state == State::Hidden && isRewardEligible(room) &&
        !room.isClearRewardCollected())) {
        activateRoom(room, tileMap, effectManager, objectPool);
    }
    if (m_state == State::Hidden || m_state == State::Collected) {
        return;
    }

    if (m_state == State::Closed) {
        const auto treasureBounds = getTreasureBounds();
        const bool overlapsTreasure = treasureBounds &&
            player.getGlobalBounds().findIntersection(*treasureBounds);
        if (overlapsTreasure && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::F)) {
            m_openHoldTime = std::min(kOpenHoldDuration, m_openHoldTime + dt);
            if (m_openHoldTime >= kOpenHoldDuration) {
                room.setClearRewardChestOpened(true);
                m_state = State::RewardVisible;
                m_rewardElapsed = 0.f;
                m_fairyRiseElapsed = 0.f;
                m_isFairyInteractable = false;
                m_fairySprites[m_selectedFairyIndex]->sprite.setPosition(
                    m_fairyStartPosition);
            }
        } else {
            m_openHoldTime = 0.f;
        }
        return;
    }

    m_rewardElapsed += dt;
    if (m_selectedFairyIndex < m_fairySprites.size() &&
        m_fairySprites[m_selectedFairyIndex]) {
        FairyVisual& fairy = *m_fairySprites[m_selectedFairyIndex];
        fairy.animator.update(dt, fairy.sprite);
        if (!m_isFairyInteractable) {
            m_fairyRiseElapsed = std::min(kFairyRiseDuration, m_fairyRiseElapsed + dt);
            const float progress = m_fairyRiseElapsed / kFairyRiseDuration;
            fairy.sprite.setPosition(m_fairyStartPosition +
                (m_fairyTargetPosition - m_fairyStartPosition) * progress);
            m_isFairyInteractable = m_fairyRiseElapsed >= kFairyRiseDuration;
        }
    }
    const auto rewardBounds = getRewardBounds();
    if (m_isFairyInteractable && rewardBounds &&
        player.getGlobalBounds().findIntersection(*rewardBounds)) {
        const RoomClearRewardConfig& reward = room.getClearRewardConfig();
        player.applyPermanentReward(reward.maxHpIncrease, reward.powerIncrease);
        room.setClearRewardCollected(true);
        m_state = State::Collected;
    }
}

void RewardChestManager::render(sf::RenderWindow& window) const {
    if (m_state == State::Hidden || m_state == State::Collected) {
        return;
    }

    if (m_state == State::Closed && m_closedTreasureSprite) {
        window.draw(*m_closedTreasureSprite);
        if (m_openHoldTime > 0.f) {
            const sf::FloatRect treasureBounds = m_closedTreasureSprite->getGlobalBounds();
            sf::RectangleShape border({ treasureBounds.size.x, 3.f });
            border.setPosition({ treasureBounds.position.x, treasureBounds.position.y - 7.f });
            border.setFillColor(sf::Color(20, 20, 20, 220));
            window.draw(border);

            sf::RectangleShape progress({ treasureBounds.size.x *
                (m_openHoldTime / kOpenHoldDuration), 3.f });
            progress.setPosition(border.getPosition());
            progress.setFillColor(sf::Color(255, 220, 80));
            window.draw(progress);
        }
        return;
    }

    if (m_state == State::RewardVisible && m_openedTreasureSprite &&
        m_fairySprites[m_selectedFairyIndex]) {
        sf::Sprite fairy = m_fairySprites[m_selectedFairyIndex]->sprite;
        if (m_isFairyInteractable) {
            fairy.move({ 0.f, std::sin(m_rewardElapsed * 4.f) * 3.f });
            window.draw(*m_openedTreasureSprite);
            window.draw(fairy);
        } else {
            // 상승 중에는 요정을 상자보다 먼저 그려 상자 내부에서 나오는 것처럼 보이게 합니다.
            window.draw(fairy);
            window.draw(*m_openedTreasureSprite);
        }
    }
}

void RewardChestManager::activateRoom(Room& room, const TileMap& tileMap,
    EffectManager& effectManager, ObjectPoolingManager& objectPool) {
    m_room = &room;
    m_openHoldTime = 0.f;
    m_rewardElapsed = 0.f;
    m_fairyRiseElapsed = 0.f;
    m_isFairyInteractable = false;
    m_state = State::Hidden;
    if (!isRewardEligible(room) || room.isClearRewardCollected() ||
        !m_closedTreasureSprite || !m_openedTreasureSprite) {
        return;
    }

    m_selectedFairyIndex = getFairyIndex(room.getClearRewardConfig().fairySize);
    if (m_selectedFairyIndex >= m_fairySprites.size() ||
        !m_fairySprites[m_selectedFairyIndex]) {
        return;
    }

    const std::vector<sf::Vector2f> candidates = room.getMonsterSpawnPositions(tileMap);
    if (candidates.empty()) {
        return;
    }
    // 바닥 후보 중 방 중심 X에 가장 가까운 위치를 선택해, 플랫폼 위가 아닌 중앙 바닥에 둡니다.
    const float centerX = tileMap.getPixelSize().x * 0.5f;
    const float groundY = tileMap.getPixelSize().y;
    const auto spawnIt = std::min_element(candidates.begin(), candidates.end(),
        [centerX, groundY](const sf::Vector2f& left, const sf::Vector2f& right) {
            const float leftScore = std::abs(left.x - centerX) +
                std::abs(left.y - groundY) * 10.f;
            const float rightScore = std::abs(right.x - centerX) +
                std::abs(right.y - groundY) * 10.f;
            return leftScore < rightScore;
        });
    const sf::Vector2f spawnPosition = *spawnIt;
    m_closedTreasureSprite->setPosition(spawnPosition);
    m_openedTreasureSprite->setPosition(spawnPosition);
    m_fairyStartPosition = { spawnPosition.x, spawnPosition.y - 8.f };
    m_fairyTargetPosition = { spawnPosition.x, spawnPosition.y - kFairyRiseHeight };
    m_fairySprites[m_selectedFairyIndex]->sprite.setPosition(
        room.isClearRewardChestOpened() ? m_fairyTargetPosition : m_fairyStartPosition);
    m_isFairyInteractable = room.isClearRewardChestOpened();
    if (!room.isClearRewardChestOpened()) {
        effectManager.spawnRewardChestMagicCircle(objectPool, spawnPosition);
    }
    m_state = room.isClearRewardChestOpened()
        ? State::RewardVisible
        : State::Closed;
}

bool RewardChestManager::isRewardEligible(const Room& room) const {
    return room.getInfo().isClear && room.isMonsterEncounterPrepared() &&
        !room.getEncounterMonsters().empty() && room.getClearRewardConfig().enabled;
}

std::optional<sf::FloatRect> RewardChestManager::getTreasureBounds() const {
    if (!m_closedTreasureSprite) {
        return std::nullopt;
    }
    return m_closedTreasureSprite->getGlobalBounds();
}

std::optional<sf::FloatRect> RewardChestManager::getRewardBounds() const {
    if (m_selectedFairyIndex >= m_fairySprites.size() ||
        !m_fairySprites[m_selectedFairyIndex]) {
        return std::nullopt;
    }
    return m_fairySprites[m_selectedFairyIndex]->sprite.getGlobalBounds();
}
