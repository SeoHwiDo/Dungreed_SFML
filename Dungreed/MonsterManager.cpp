#include "MonsterManager.h"

#include "Player.h"
#include "TileMap.h"
#include "Collision.h"
#include <vector>

void MonsterManager::update(float dt, Player& player,
    ObjectPoolingManager& objectPool, const TileMap& tileMap) const {
    std::vector<Monster*> finished;
    objectPool.forEachActiveMonster([&](Monster& monster) {
        monster.update(dt, player);
        Collision::resolveMapCollision(monster, tileMap);
        if (monster.readyForPoolRelease()) {
            finished.push_back(&monster);
        }
    });
    for (Monster* monster : finished) {
        objectPool.releaseMonster(monster);
    }
}
