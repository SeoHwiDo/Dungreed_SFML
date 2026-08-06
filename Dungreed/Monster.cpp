#include "Monster.h"
#include "ResourceManager.h"
#include <iostream>
#include <cmath>   // std::sqrt
#include <cstdlib> // std::rand

void Monster::init(const std::string& atlasKey) {
    Actor::init(atlasKey);

    auto& resMgr = ResourceManager::getInstance();
    std::vector<std::string> allAnims = resMgr.getAnimationNames(atlasKey);

    for (const auto& animName : allAnims) {
        // 전체 애니메이션 중, 현재 몬스터의 타입 이름이 포함된 경우만 필터링
        if (animName.find(m_type) != std::string::npos) {
            const auto* frames = resMgr.getAnimationFrames(atlasKey, animName);
            if (frames) {
                // 단발성 애니메이션 추론 (공격, 사망)
                bool isLoop = true;
                if (animName.find("Attack") != std::string::npos ||
                    animName.find("Dead") != std::string::npos) {
                    isLoop = false;
                }

                AnimationClip clip(frames, 0.1f, isLoop);
                animator.addAnimation(animName, clip);
            }
        }
    }
    animator.play(m_type + "_Idle");
}

void Monster::update(float dt) {

}
