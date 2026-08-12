#pragma once

#include "Boss.h"

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <random>
#include <vector>

class Effect;
class Projectile;
struct BossData;

enum class SkelBossPattern { None, HandLaser, RotatingBullet, SwordFan };

class SkelBoss final : public Boss {
  public:
    explicit SkelBoss(const BossData &data);

    void init(const std::string &atlasKey = "Boss") override;
    void update(float dt, Player &player, ObjectPoolingManager &objectPool, EffectManager &effectManager, const TileMap &tileMap) override;
    void render(sf::RenderWindow &window) override;

    SkelBossPattern getCurrentPattern() const { return m_currentPattern; }
    bool isDeathSequenceFinished() const;

  private:
    enum class State { Summoning, Idle, Attacking, Dead };

    static constexpr float kPostPatternIdleDuration = 3.f;
    static constexpr float kSwordAimDuration = 1.5f;
    static constexpr float kSwordSummonFxDuration = 0.16f;

    struct PendingSwordSpawn {
        sf::Vector2f position{};
        float remainingFxTime = 0.f;
    };

    struct DeathFragment {
        sf::Sprite sprite;
        sf::Vector2f velocity{};
        float angularVelocity = 0.f;
    };

    State m_state = State::Summoning;
    SkelBossPattern m_currentPattern = SkelBossPattern::None;
    std::array<std::uint64_t, 3> m_patternLastUsed{};
    std::uint64_t m_patternUseSequence = 0;
    float m_stateTimer = 0.f;
    std::string m_handLaserWeaponId;
    std::string m_rotatingBulletWeaponId;
    std::string m_swordFanWeaponId;
    float m_patternTimer = 0.f;

    std::optional<sf::Sprite> m_leftHand;
    std::optional<sf::Sprite> m_rightHand;
    std::optional<sf::Sprite> m_backSprite;
    Animator m_leftHandAnimator;
    Animator m_rightHandAnimator;
    Animator m_backAnimator;
    sf::Vector2f m_leftHandPosition{};
    sf::Vector2f m_rightHandPosition{};

    bool m_attackingWithRightHand = false;
    int m_completedLaserHands = 0;
    sf::Vector2f m_lockedLaserTarget{};
    sf::RectangleShape m_laserBeam;
    std::optional<sf::Sprite> m_laserBody;
    std::optional<sf::Sprite> m_laserHead;
    Animator m_laserBodyAnimator;
    Animator m_laserHeadAnimator;
    bool m_laserActive = false;
    bool m_laserDamageConsumed = false;

    float m_bulletIntervalTimer = 0.f;
    float m_bulletRotation = 0.f;
    float m_backParticleTimer = 0.f;
    std::mt19937 m_backParticleRandom{std::random_device{}()};

    std::shared_ptr<Equip> m_handLaserWeapon;
    std::shared_ptr<Equip> m_bulletWeapon;
    std::shared_ptr<Equip> m_swordWeapon;

    float m_handMotionTime = 0.f;
    float m_leftHandY = 0.f;
    float m_rightHandY = 0.f;
    float m_lockedHandY = 0.f;

    std::vector<Projectile *> m_swords;
    std::vector<Effect *> m_swordChargeEffects;
    std::vector<PendingSwordSpawn> m_pendingSwordSpawns;
    bool m_swordsLaunched = false;
    bool m_swordsReadyForAim = false;
    float m_swordAimTimer = 0.f;
    std::vector<DeathFragment> m_deathFragments;
    sf::Vector2f m_leftHandDeathVelocity{};
    sf::Vector2f m_rightHandDeathVelocity{};
    float m_deathSequenceElapsed = 0.f;
    bool m_isDeathSequenceActive = false;

    void configureAnimations();
    void configureHands();
    void configureLaser();
    void configurePatternWeapons();
    void updateBackVisual(float dt);
    void updateBackParticles(float dt, ObjectPoolingManager &objectPool);
    void updateHands(float dt, const TileMap &tileMap);
    void updateHandPositions(const TileMap &tileMap);
    void setHandAttackAnimation(bool rightHand);

    SkelBossPattern choosePattern(const Player &player, const TileMap &tileMap) const;
    void startPattern(SkelBossPattern pattern, const Player &player, ObjectPoolingManager &objectPool);
    void finishPattern();

    void updateHandLaser(float dt, Player &player);
    void beginLaserHand(const Player &player, bool rightHand);
    void updateRotatingBullets(float dt, ObjectPoolingManager &objectPool);
    void fireRotatingCross(ObjectPoolingManager &objectPool);
    void updateSwordFan(float dt, Player &player, ObjectPoolingManager &objectPool, EffectManager &effectManager, const TileMap &tileMap);
    void summonSwordFan(ObjectPoolingManager &objectPool);
    void spawnSword(const sf::Vector2f &position, ObjectPoolingManager &objectPool);
    void stopSwordChargeEffects(ObjectPoolingManager &objectPool);
    void updateManagedSwords(float dt, Player &player, ObjectPoolingManager &objectPool,
        const TileMap &tileMap);
    bool resolveSwordGroundCollision(Projectile &sword, const TileMap &tileMap) const;
    void clearManagedSwords(ObjectPoolingManager &objectPool);
    void beginDeathSequence(ObjectPoolingManager &objectPool);
    void updateDeathSequence(float dt);

    sf::Vector2f getMouthPosition() const;
    static std::size_t patternIndex(SkelBossPattern pattern);
    void markPatternUsed(SkelBossPattern pattern);
    static sf::Vector2f normalized(const sf::Vector2f &vector);
    static float directionAngle(const sf::Vector2f &direction);
};
