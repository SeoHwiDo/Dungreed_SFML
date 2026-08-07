#pragma once

#include "ObjectPoolingManager.h"

class Player;
class TileMap;

/// 몬스터 객체를 직접 소유하지 않고, 풀에 있는 활성 몬스터의 AI·물리 업데이트만 조정합니다.
class MonsterManager {
public:
    /// 모든 활성 몬스터를 갱신한 뒤 벽 충돌을 적용합니다. 실제 공격 판정은 CombatManager가 담당합니다.
    void update(float dt, Player& player, ObjectPoolingManager& objectPool, const TileMap& tileMap) const;
};
