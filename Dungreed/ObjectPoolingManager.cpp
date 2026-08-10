#include "ObjectPoolingManager.h"

#include "GameDataManager.h"

void ObjectPoolingManager::prewarmMonsters(std::size_t count,
    const std::string& type, Actor::Status status, const std::string& atlasKey, MonsterBehaviorConfig behavior) {
    for (std::size_t i = 0; i < count; ++i) {
        MonsterSlot slot;
        slot.object = std::make_unique<Monster>(type, status, atlasKey, behavior);
        slot.active = false;
        m_monsters.push_back(std::move(slot));
        enqueueInactiveMonster(m_monsters.size() - 1);
    }
}

void ObjectPoolingManager::prewarmProjectiles(std::size_t count) {
    for (std::size_t i = 0; i < count; ++i) {
        ProjectileSlot slot;
        slot.object = std::make_unique<Projectile>();
        slot.active = false;
        m_projectiles.push_back(std::move(slot));
        enqueueInactiveProjectile(m_projectiles.size() - 1);
    }
}

void ObjectPoolingManager::prewarmEffects(std::size_t count) {
    for (std::size_t index = 0; index < count; ++index) {
        EffectSlot slot;
        slot.object = std::make_unique<Effect>();
        slot.active = false;
        m_effects.push_back(std::move(slot));
        enqueueInactiveEffect(m_effects.size() - 1);
    }
}
void ObjectPoolingManager::prewarmFromGameData(const GameDataManager& gameData,
    float reserveRatio) {
    const PoolPrewarmPlan plan = gameData.createPoolPrewarmPlan(reserveRatio);
    for (const MonsterPrewarmData& monster : plan.monsters) {
        prewarmMonsters(monster.count, monster.monsterId, monster.status,
            monster.atlasKey, monster.behavior);
    }
    prewarmProjectiles(plan.projectileCount);
    // 플레이어 스윙과 다수 적중 시의 SlashFX를 위해 기본 효과 풀을 준비합니다.
    prewarmEffects(16);
}

Monster* ObjectPoolingManager::acquireMonster(const std::string& type,
    Actor::Status status, const std::string& atlasKey, MonsterBehaviorConfig behavior) {
    if (MonsterSlot* slot = dequeueInactiveMonster()) {
        slot->object->resetForReuse(type, status, atlasKey, behavior);
        slot->active = true;
        return slot->object.get();
    }

    // 비활성 큐가 비어 있을 때만 새 슬롯을 확장합니다.
    MonsterSlot slot;
    slot.object = std::make_unique<Monster>(type, status, atlasKey, behavior);
    slot.active = true;
    Monster* result = slot.object.get();
    m_monsters.push_back(std::move(slot));
    return result;
}

void ObjectPoolingManager::releaseMonster(Monster* monster) {
    if (!monster) {
        return;
    }
    for (std::size_t index = 0; index < m_monsters.size(); ++index) {
        MonsterSlot& slot = m_monsters[index];
        if (slot.object.get() == monster) {
            if (!slot.active) {
                return;
            }
            slot.active = false;
            enqueueInactiveMonster(index);
            return;
        }
    }
}

void ObjectPoolingManager::forEachActiveMonster(
    const std::function<void(Monster&)>& operation) {
    for (MonsterSlot& slot : m_monsters) {
        if (slot.active && slot.object) {
            operation(*slot.object);
        }
    }
}

Projectile* ObjectPoolingManager::acquireProjectile(
    const ProjectileSpawnRequest& request) {
    if (ProjectileSlot* slot = dequeueInactiveProjectile()) {
        slot->object->activate(request);
        slot->active = true;
        return slot->object.get();
    }

    // 미리 확보한 비활성 슬롯이 모두 사용 중일 때만 새 객체를 생성합니다.
    ProjectileSlot slot;
    slot.object = std::make_unique<Projectile>();
    slot.object->activate(request);
    slot.active = true;
    Projectile* result = slot.object.get();
    m_projectiles.push_back(std::move(slot));
    return result;
}

void ObjectPoolingManager::releaseProjectile(Projectile* projectile) {
    if (!projectile) {
        return;
    }
    for (std::size_t index = 0; index < m_projectiles.size(); ++index) {
        ProjectileSlot& slot = m_projectiles[index];
        if (slot.object.get() == projectile) {
            if (!slot.active) {
                return;
            }
            // 밴시 투사체는 Trail을 끝까지 보인 후, 다음 반환 요청에서 실제 비활성 슬롯이 됩니다.
            if (slot.object->beginReturnTrail()) {
                return;
            }
            slot.object->deactivate();
            slot.active = false;
            enqueueInactiveProjectile(index);
            return;
        }
    }
}

Effect* ObjectPoolingManager::acquireEffect(const EffectSpawnRequest& request) {
    if (EffectSlot* slot = dequeueInactiveEffect()) {
        if (!slot->object->activate(request)) {
            enqueueInactiveEffect(static_cast<std::size_t>(slot - m_effects.data()));
            return nullptr;
        }
        slot->active = true;
        return slot->object.get();
    }

    EffectSlot slot;
    slot.object = std::make_unique<Effect>();
    if (!slot.object->activate(request)) {
        return nullptr;
    }
    slot.active = true;
    Effect* result = slot.object.get();
    m_effects.push_back(std::move(slot));
    return result;
}

void ObjectPoolingManager::releaseEffect(Effect* effect) {
    if (!effect) {
        return;
    }
    for (std::size_t index = 0; index < m_effects.size(); ++index) {
        EffectSlot& slot = m_effects[index];
        if (slot.object.get() == effect && slot.active) {
            slot.active = false;
            enqueueInactiveEffect(index);
            return;
        }
    }
}

void ObjectPoolingManager::enqueueInactiveMonster(std::size_t index) {
    m_inactiveMonsterSlots.push({ index, m_nextAvailableOrder++ });
}

void ObjectPoolingManager::enqueueInactiveProjectile(std::size_t index) {
    m_inactiveProjectileSlots.push({ index, m_nextAvailableOrder++ });
}

void ObjectPoolingManager::enqueueInactiveEffect(std::size_t index) {
    m_inactiveEffectSlots.push({ index, m_nextAvailableOrder++ });
}

ObjectPoolingManager::MonsterSlot* ObjectPoolingManager::dequeueInactiveMonster() {
    while (!m_inactiveMonsterSlots.empty()) {
        const std::size_t index = m_inactiveMonsterSlots.top().index;
        m_inactiveMonsterSlots.pop();
        if (index < m_monsters.size() && !m_monsters[index].active && m_monsters[index].object) {
            return &m_monsters[index];
        }
    }
    return nullptr;
}

ObjectPoolingManager::ProjectileSlot* ObjectPoolingManager::dequeueInactiveProjectile() {
    while (!m_inactiveProjectileSlots.empty()) {
        const std::size_t index = m_inactiveProjectileSlots.top().index;
        m_inactiveProjectileSlots.pop();
        if (index < m_projectiles.size() && !m_projectiles[index].active && m_projectiles[index].object) {
            return &m_projectiles[index];
        }
    }
    return nullptr;
}

ObjectPoolingManager::EffectSlot* ObjectPoolingManager::dequeueInactiveEffect() {
    while (!m_inactiveEffectSlots.empty()) {
        const std::size_t index = m_inactiveEffectSlots.top().index;
        m_inactiveEffectSlots.pop();
        if (index < m_effects.size() && !m_effects[index].active && m_effects[index].object) {
            return &m_effects[index];
        }
    }
    return nullptr;
}

void ObjectPoolingManager::forEachActiveProjectile(
    const std::function<void(Projectile&)>& operation) {
    for (ProjectileSlot& slot : m_projectiles) {
        if (slot.active && slot.object) {
            operation(*slot.object);
        }
    }
}

void ObjectPoolingManager::forEachActiveEffect(
    const std::function<void(Effect&)>& operation) const {
    for (const EffectSlot& slot : m_effects) {
        if (slot.active && slot.object) {
            operation(*slot.object);
        }
    }
}

void ObjectPoolingManager::render(sf::RenderWindow& window) const {
    for (const MonsterSlot& slot : m_monsters) {
        if (slot.active && slot.object) {
            slot.object->render(window);
        }
    }
    for (const ProjectileSlot& slot : m_projectiles) {
        if (slot.active && slot.object) {
            slot.object->render(window);
        }
    }
}
