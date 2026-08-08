#pragma once
#include<SFML/Graphics.hpp>
#include<string>
#include<unordered_map>
#include<vector>

enum class AnimState
{
    Idle,
    Run,
    Jump,
    Attack,
    Dead
};

//애니메이션 객체 정보
struct AnimationClip {
    const std::vector<sf::IntRect>* frames;//애니메이션 각 프레임
    float frameDuration;//각 프레임 출력되는 시간
    bool isLoop;//반복 여부

    /// 비어 있는 기본 클립을 만듭니다. 실제 사용 전 addAnimation으로 프레임을 등록해야 합니다.
    AnimationClip() :frames(nullptr),frameDuration(0.5f), isLoop(true) {};
    /// 아틀라스 프레임 배열과 재생 규칙으로 클립을 구성합니다.
    AnimationClip(const std::vector<sf::IntRect>* f, float duration, bool loop):frames(f),frameDuration(duration),isLoop(loop) {};
};

class Animator{
private:
    std::unordered_map<std::string, AnimationClip> m_animations;
    std::string m_currentAnimation;
    float m_speedMultiplier = 1.0f; // 추가: 애니메이션 배속 비율
    float m_currentTime;
    size_t m_currentFrame;
    bool m_isPlaying;
public:
    /// 재생 중인 클립이 없는 애니메이터를 생성합니다.
    Animator(): m_currentTime(0.f), m_currentFrame(0), m_isPlaying(false) {}
    /// 이름으로 애니메이션 클립을 등록합니다. Actor 초기화 단계에서 호출합니다.
    void addAnimation(const std::string& name, const AnimationClip& clip);
    /// 등록된 클립을 첫 프레임부터 재생합니다. 존재하지 않는 이름은 경고만 출력합니다.
    void play(const std::string& name);
    /// 현재 선택된 애니메이션 이름을 반환합니다. 중복 재생 방지에 사용합니다.
    inline const std::string& getCurrentAnimation() const {return m_currentAnimation;}
    /// 현재 프레임을 유지한 채 재생을 멈춥니다.
    void stop();

    /// 지정한 이름의 클립이 등록되어 있는지 확인합니다.
    bool hasAnimation(const std::string& name) const;

    /// 매 프레임 경과 시간을 반영해 대상 스프라이트의 TextureRect를 갱신합니다.
    void update(float dt, sf::Sprite& sprite);

    /// 비루프 애니메이션이 마지막 프레임까지 끝났는지 반환합니다. 사망/공격 전환에 사용합니다.
    bool isFinished() const;

};

