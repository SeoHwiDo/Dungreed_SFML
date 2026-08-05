#include "Animator.h"
#include<iostream>

void Animator::addAnimation(const std::string& name, const AnimationClip& clip) {
    if (clip.frames != nullptr && !clip.frames->empty()) {//참조해온 정보가 존재하는지 확인
        m_animations[name] = clip;
    } else {
        std::cerr << "[Animator] 경고: 유효하지 않은 프레임 데이터가 등록되었습니다. (" << name << ")\n";
    }
}

void Animator::Play(const std::string& name) {
    if (m_currentAnimation == name && m_isPlaying) return;
    if (m_animations.find(name) != m_animations.end()) {
        //정상적으로 존재하는 애니메이션일때
        m_currentAnimation = name;
        m_currentTime = 0.f;
        m_currentFrame = 0;
        m_isPlaying = true;
    } else {
        std::cerr << "[Animator] 경고: 애니메이션을 찾을 수 없습니다. :" << name << "\n";
    }
}

void Animator::Stop() {
    m_isPlaying = false;
    m_currentFrame = 0;
    m_currentTime = 0.f;
}
void Animator::Update(float dt, sf::Sprite& sprite) {
    if (!m_isPlaying || m_currentAnimation.empty()) return;

    const AnimationClip& clip = m_animations[m_currentAnimation];

    // 포인터 유효성 검사
    if (!clip.frames || clip.frames->empty()) return;

    // 객체 자신의 독립적인 시간에 dt를 누적 (비동기적 프레임 갱신)
    m_currentTime += dt;

    // 프레임 전환 시간이 지났을 경우
    while (m_currentTime >= clip.frameDuration) {
        m_currentTime -= clip.frameDuration;
        m_currentFrame++;

        // 마지막 프레임 도달 시 처리
        if (m_currentFrame >= clip.frames->size()) {
            if (clip.isLoop) {
                m_currentFrame = 0; // 루프인 경우 처음으로
            } else {
                m_currentFrame = clip.frames->size() - 1; // 루프가 아니면 마지막 프레임에서 정지
                m_isPlaying = false;
            }
        }
    }

    // SFML 3.1.0 기준: Sprite에 현재 프레임의 영역(IntRect) 적용
    sprite.setTextureRect((*clip.frames)[m_currentFrame]);
}
bool Animator::IsFinished() const {
    return !m_isPlaying;
}