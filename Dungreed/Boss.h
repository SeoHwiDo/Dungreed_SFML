#pragma once

#include "Actor.h"

#include <string>

class EffectManager;
class ObjectPoolingManager;
class Player;
class TileMap;

/// 모든 보스가 공유하는 고정 배치, 소환 연출, 이름과 체력 정보를 관리합니다.
class Boss : public Actor {
  public:
    Boss(std::string displayName, Status initialStatus);
    ~Boss() override;

    void init(const std::string &atlasKey) override;
    void placeAtMapCenter(const TileMap &tileMap);
    void beginSummon(float duration = 1.8f);

    virtual void update(float dt, Player &player, ObjectPoolingManager &objectPool, EffectManager &effectManager, const TileMap &tileMap) = 0;
    virtual void renderBehindTiles(sf::RenderWindow &window) const;

    void takeDamage(float damage, const sf::Vector2f &attackerPosition, float knockbackMultiplier) override;

    const std::string &getDisplayName() const { return m_displayName; }
    void setDisplayName(std::string displayName);
    bool isSummoning() const { return m_summonTimer > 0.f; }
    float getSummonProgress() const;

  protected:
    void updateBossBase(float dt);

  private:
    std::string m_displayName;
    float m_summonTimer = 0.f;
    float m_summonDuration = 0.f;
};
