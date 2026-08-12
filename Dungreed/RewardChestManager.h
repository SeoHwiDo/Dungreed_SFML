#pragma once

#include <SFML/Graphics.hpp>

#include <array>
#include <optional>

#include "Animator.h"

class Player;
class Room;
class TileMap;
class EffectManager;
class ObjectPoolingManager;

/// 전투방 클리어 상자, F키 개방, room_data.json 기반 요정 보상을 관리합니다.
class RewardChestManager {
  public:
    static RewardChestManager &getInstance() {
        static RewardChestManager instance;
        return instance;
    }

    RewardChestManager(const RewardChestManager &) = delete;
    RewardChestManager &operator=(const RewardChestManager &) = delete;
    /// 닫힌/열린 보물상자와 S·M·L·XL 요정 스프라이트를 준비합니다.
    bool init();
    /// 현재 방의 클리어 상태, 상자 상호작용, 요정 보상 충돌을 갱신합니다.
    void update(float dt, Room &room, const TileMap &tileMap, Player &player, EffectManager &effectManager, ObjectPoolingManager &objectPool);
    /// 현재 방에 표시해야 하는 닫힌/열린 상자와 요정을 렌더링합니다.
    void render(sf::RenderWindow &window) const;
    /// 장면 이탈 시 이전 Room 포인터와 보상 상태를 비웁니다.
    void reset();

  private:
    RewardChestManager() = default;
    ~RewardChestManager() = default;
    enum class State { Hidden, Closed, RewardVisible, Collected };

    static constexpr float kOpenHoldDuration = 1.f;
    static constexpr float kFairyRiseHeight = 72.f;
    static constexpr float kFairyRiseDuration = 0.6f;

    struct FairyVisual {
        sf::Sprite sprite;
        Animator animator;
    };

    Room *m_room = nullptr;
    std::optional<sf::Sprite> m_closedTreasureSprite;
    std::optional<sf::Sprite> m_openedTreasureSprite;
    std::array<std::optional<FairyVisual>, 4> m_fairySprites;
    std::size_t m_selectedFairyIndex = 0;
    State m_state = State::Hidden;
    float m_openHoldTime = 0.f;
    float m_rewardElapsed = 0.f;
    float m_fairyRiseElapsed = 0.f;
    sf::Vector2f m_fairyStartPosition{};
    sf::Vector2f m_fairyTargetPosition{};
    bool m_isFairyInteractable = false;

    /// 방 진입 시 방에 저장된 보상 구성에 맞춰 상자와 요정을 배치합니다.
    void activateRoom(Room &room, const TileMap &tileMap, EffectManager &effectManager, ObjectPoolingManager &objectPool);
    /// 전투방이며 room_data.json에 clearReward가 지정되었는지 검사합니다.
    bool isRewardEligible(const Room &room) const;
    std::optional<sf::FloatRect> getTreasureBounds() const;
    std::optional<sf::FloatRect> getRewardBounds() const;
};
