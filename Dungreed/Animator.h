#pragma once
#include<SFML/Graphics.hpp>
#include<string>
#include<unordered_map>
#include<vector>


//애니메이션 객체 정보
struct AnimationClip {
    const std::vector<sf::IntRect>* frames;//애니메이션 각 프레임
    float frameDuration;//각 프레임 출력되는 시간
    bool isLoop;//반복 여부

    AnimationClip() :frames(nullptr),frameDuration(0.1f), isLoop(true) {};
    AnimationClip(const std::vector<sf::IntRect>* f, float duration, bool loop):frames(f),frameDuration(duration),isLoop(loop) {};
};

class Animator{
private:
    std::unordered_map<std::string, AnimationClip> m_animations;
    std::string m_currentAnimation;

    float m_currentTime;
    size_t m_currentFrame;
    bool m_isPlaying;
public:
    Animator(): m_currentTime(0.f), m_currentFrame(0), m_isPlaying(false) {}
    void addAnimation(const std::string& name, const AnimationClip& clip);
    void Play(const std::string& name);

    // 애니메이션 정지
    void Stop();

    // 매 프레임 호출되어 스프라이트의 TextureRect를 갱신
    void Update(float dt, sf::Sprite& sprite);

    // 현재 애니메이션이 끝났는지 확인 (루프가 아닌 애니메이션에 유용)
    bool IsFinished() const;

};

