#pragma once

#include <SFML/Graphics.hpp>

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

constexpr std::string_view kPlayerAtlasPath = "resources/images/player_atlas.png";
constexpr std::string_view kMonsterAtlasPath = "resources/images/monster_atlas.png";
constexpr std::string_view kBossAtlasPath = "resources/images/boss_atlas.png";
constexpr std::string_view kTileMapAtlasPath = "resources/images/tilemap_atlas.png";
constexpr std::string_view kEquipAtlasPath = "resources/images/equip_atlas.png";
constexpr std::string_view kProjectileAtlasPath = "resources/images/projectile_atlas.png";

constexpr std::string_view kPlayerAtlasJsonPath = "resources/images/player_atlas.json";
constexpr std::string_view kMonsterAtlasJsonPath = "resources/images/monster_atlas.json";
constexpr std::string_view kBossAtlasJsonPath = "resources/images/boss_atlas.json";
constexpr std::string_view kTileMapAtlasJsonPath = "resources/images/tilemap_atlas.json";
constexpr std::string_view kEquipAtlasJsonPath = "resources/images/equip_atlas.json";
constexpr std::string_view kProjectileAtlasJsonPath = "resources/images/projectile_atlas.json";
constexpr std::string_view kBackgroundPath = "resources/images/Backgrounds/";

struct AtlasData {
    sf::Texture texture;
    std::unordered_map<std::string, sf::IntRect> frameRects;
    std::unordered_map<std::string, sf::Vector2f> framePivots;
    std::unordered_map<std::string, sf::Vector2u> frameSourceSizes;
    std::unordered_map<std::string, std::vector<sf::IntRect>> animations;
};

class ResourceManager {
public:
    static ResourceManager& getInstance() {
        static ResourceManager instance;
        return instance;
    }

    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;

    bool loadAtlas(const std::string& atlasKey, const std::string& jsonPath,
        const std::string& imagePath);
    const sf::Texture* getAtlasTexture(const std::string& atlasKey) const;
    const sf::IntRect* getFrameRect(const std::string& atlasKey,
        const std::string& frameName) const;
    std::optional<sf::Vector2f> getFramePivot(const std::string& atlasKey,
        const std::string& frameName) const;
    std::optional<sf::Vector2u> getFrameSourceSize(const std::string& atlasKey,
        const std::string& frameName) const;
    const std::vector<sf::IntRect>* getAnimationFrames(const std::string& atlasKey,
        const std::string& animationName) const;
    std::vector<std::string> getAnimationNames(const std::string& atlasKey) const;

private:
    ResourceManager() = default;
    ~ResourceManager() = default;

    std::string extractAnimationName(const std::string& frameName) const;

    std::unordered_map<std::string, std::unique_ptr<AtlasData>> m_atlases;
};