#include "ObjectPoolingManager.h"

Monster* ObjectPoolingManager::acquireMonster(const std::string& type,
    Actor::Status status, const std::string& atlasKey) {
    for (MonsterSlot& slot : m_monsters) {
        if (!slot.active && slot.object) {
            slot.object->resetForReuse(type, status, atlasKey);
            slot.active = true;
            return slot.object.get();
        }
    }

    MonsterSlot slot;
    slot.object = std::make_unique<Monster>(type, status, atlasKey);
    slot.active = true;
    Monster* result = slot.object.get();
    m_monsters.push_back(std::move(slot));
    return result;
}

void ObjectPoolingManager::releaseMonster(Monster* monster) {
    if (!monster) {
        return;
    }
    for (MonsterSlot& slot : m_monsters) {
        if (slot.object.get() == monster) {
            slot.active = false;
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
    for (ProjectileSlot& slot : m_projectiles) {
        if (!slot.active && slot.object) {
            slot.object->activate(request);
            slot.active = true;
            return slot.object.get();
        }
    }

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
    for (ProjectileSlot& slot : m_projectiles) {
        if (slot.object.get() == projectile) {
            slot.object->deactivate();
            slot.active = false;
            return;
        }
    }
}

void ObjectPoolingManager::forEachActiveProjectile(
    const std::function<void(Projectile&)>& operation) {
    for (ProjectileSlot& slot : m_projectiles) {
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
