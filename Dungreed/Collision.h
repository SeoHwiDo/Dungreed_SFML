#pragma once
#include "Actor.h"

class collision {
public:
    // 맵(타일)과의 충돌 여부 및 중력/물리 처리 적용
    static bool mapCheck(Actor* actor);

    // 공격 판정 확인
    static bool attackCheck(Actor* actor);

    // 피격 판정 확인 (피격당한 대상 객체 반환)
    static Actor* hitCheck();
};