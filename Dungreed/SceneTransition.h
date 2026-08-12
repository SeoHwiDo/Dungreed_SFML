#pragma once

#include <SFML/Graphics.hpp>

#include <functional>
#include <optional>
#include <string>
#include <vector>

// 게임의 큰 화면 단위를 명시적으로 구분합니다. 방 이동은 Dungeon 장면 내부의 이동입니다.
enum class GameScene { Title, TrainingVillage, Dungeon };

// 화면을 완전히 가린 다음, 한 프레임에 하나씩 로딩 작업을 실행하고 다시 화면을 엽니다.
// 동기식 SFML 로딩도 이 오버레이 아래에서 실행되므로 장면이 바뀌는 순간이 노출되지 않습니다.
class SceneTransition {
  public:
    using LoadTask = std::function<bool()>;

    bool init(const sf::Font &font, const sf::Vector2u &size);
    bool begin(GameScene destination, std::string destinationName, std::vector<LoadTask> loadTasks, std::function<void()> onLoaded);
    void update(float dt);
    void render(sf::RenderWindow &window) const;

    bool isActive() const;
    bool hasFailed() const { return m_failed; }
    const std::string &getErrorMessage() const { return m_errorMessage; }

  private:
    enum class Phase { Idle, FadeOut, Loading, FadeIn };

    void createCoverImage(const sf::Vector2u &size);
    float getOpacity() const;

    static constexpr float kFadeDuration = 0.35f;

    Phase m_phase = Phase::Idle;
    GameScene m_destination = GameScene::Title;
    float m_phaseElapsed = 0.f;
    std::size_t m_nextTask = 0;
    std::vector<LoadTask> m_loadTasks;
    std::function<void()> m_onLoaded;
    std::optional<sf::Texture> m_coverTexture;
    std::optional<sf::Sprite> m_coverSprite;
    std::optional<sf::Text> m_loadingText;
    std::string m_destinationName;
    bool m_failed = false;
    std::string m_errorMessage;
};
