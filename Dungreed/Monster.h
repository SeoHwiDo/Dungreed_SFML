#pragma once

#include <string>
#include <utility>

// Temporary gameplay entity owned by Room. This can later be replaced by an
// object borrowed from MonsterPoolingManager without changing Room's API.
class Monster {
public:
    explicit Monster(std::string type) : m_type(std::move(type)) {}

    const std::string& getType() const { return m_type; }

private:
    std::string m_type;
};
