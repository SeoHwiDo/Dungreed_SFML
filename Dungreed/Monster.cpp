#include "Monster.h"

Monster::Monster(std::string type, Status _status, sf::Sprite monsterSprite) {
    m_type = type;
    status = _status;
    sprite = monsterSprite;
}

void Monster::update(float dt) {

}
