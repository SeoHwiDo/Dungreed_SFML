#pragma once

#include <SFML/Graphics.hpp>

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

enum class AnimState {
    Idle,
    Run,
    Jump,
    Attack,
    Dead
};

struct AnimationClip {
    const std::vector<sf::IntRect>* frames = nullptr;
    float frameDuration = 0.5f;
    bool isLoop = true;

    AnimationClip() = default;
    AnimationClip(const std::vector<sf::IntRect>* animationFrames, float duration,
        bool loop)
        : frames(animationFrames), frameDuration(duration), isLoop(loop) {}
};

class Animator {
public:
    void addAnimation(const std::string& name, const AnimationClip& clip);
    void play(const std::string& name);
    const std::string& getCurrentAnimation() const { return m_currentAnimation; }
    void stop();

    bool hasAnimation(const std::string& name) const;
    void update(float dt, sf::Sprite& sprite);
    bool isFinished() const;
    bool isOnLastFrame() const;

private:
    std::unordered_map<std::string, AnimationClip> m_animations;
    std::string m_currentAnimation;
    float m_speedMultiplier = 1.f;
    float m_currentTime = 0.f;
    std::size_t m_currentFrame = 0;
    bool m_isPlaying = false;
};