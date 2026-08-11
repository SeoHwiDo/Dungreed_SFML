#include "ResourceManager.h"

#include <fstream>
#include <iostream>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

std::string ResourceManager::extractAnimationName(const std::string& frameName) const {
    const std::size_t slashPosition = frameName.find_last_of('/');
    const std::string name = slashPosition == std::string::npos
        ? frameName
        : frameName.substr(slashPosition + 1);

    const std::size_t hyphenPosition = name.find_last_of('-');
    return hyphenPosition == std::string::npos
        ? name
        : name.substr(0, hyphenPosition);
}

bool ResourceManager::loadAtlas(const std::string& atlasKey, const std::string& jsonPath,
    const std::string& imagePath) {
    auto atlas = std::make_unique<AtlasData>();
    if (!atlas->texture.loadFromFile(imagePath)) {
        std::cerr << "텍스처 로드 실패: " << imagePath << '\n';
        return false;
    }

    std::ifstream jsonFile(jsonPath);
    if (!jsonFile) {
        std::cerr << "JSON 파일 로드 실패: " << jsonPath << '\n';
        return false;
    }

    json root;
    jsonFile >> root;
    if (!root.contains("frames") || !root["frames"].is_object()) {
        std::cerr << "프레임 데이터가 없습니다: " << jsonPath << '\n';
        return false;
    }

    for (const auto& [frameName, frameData] : root["frames"].items()) {
        const auto& frame = frameData.at("frame");
        const sf::IntRect rect{
            { frame.at("x").get<int>(), frame.at("y").get<int>() },
            { frame.at("w").get<int>(), frame.at("h").get<int>() }
        };
        atlas->frameRects.emplace(frameName, rect);

        sf::Vector2f sourceSize{
            static_cast<float>(rect.size.x),
            static_cast<float>(rect.size.y)
        };
        if (frameData.contains("sourceSize")) {
            const auto& sourceSizeData = frameData.at("sourceSize");
            const sf::Vector2u sourceSizePixels{
                sourceSizeData.at("w").get<unsigned int>(),
                sourceSizeData.at("h").get<unsigned int>()
            };
            atlas->frameSourceSizes.emplace(frameName, sourceSizePixels);
            sourceSize = {
                static_cast<float>(sourceSizePixels.x),
                static_cast<float>(sourceSizePixels.y)
            };
        }

        sf::Vector2f pivot{ rect.size.x / 2.f, rect.size.y / 2.f };
        if (frameData.contains("pivot") && frameData.contains("spriteSourceSize")) {
            const auto& pivotData = frameData.at("pivot");
            const auto& spriteSourceSize = frameData.at("spriteSourceSize");
            pivot = {
                pivotData.at("x").get<float>() * sourceSize.x -
                    spriteSourceSize.at("x").get<float>(),
                pivotData.at("y").get<float>() * sourceSize.y -
                    spriteSourceSize.at("y").get<float>()
            };
        }
        atlas->framePivots.emplace(frameName, pivot);

        const std::string animationName = extractAnimationName(frameName);
        atlas->animations[animationName].push_back(rect);
        atlas->animationPivots[animationName].push_back(pivot);
    }

    m_atlases[atlasKey] = std::move(atlas);
    return true;
}

bool ResourceManager::loadFont(const std::string& fontKey, const std::string& fontPath) {
    auto font = std::make_unique<sf::Font>();
    if (!font->openFromFile(fontPath)) {
        std::cerr << "[ResourceManager] 폰트 로드 실패: " << fontPath << '\n';
        return false;
    }
    m_fonts[fontKey] = std::move(font);
    return true;
}

bool ResourceManager::loadDefaultFont(const std::string& fontPath) {
    return loadFont(std::string(kDefaultFontKey), fontPath);
}

bool ResourceManager::loadSharedGameplayResources() {
    return loadAtlas("Player", std::string(kPlayerAtlasJsonPath), std::string(kPlayerAtlasPath)) &&
        loadAtlas("TileMap", std::string(kTileMapAtlasJsonPath), std::string(kTileMapAtlasPath)) &&
        loadAtlas("Equip", std::string(kEquipAtlasJsonPath), std::string(kEquipAtlasPath));
}

bool ResourceManager::loadTitleResources() {
    const std::string backgroundPath(kBackgroundPath);
    return loadDefaultFont(std::string(kDefaultFontPath)) &&
        loadStandaloneSprite("TitleLogo", backgroundPath + "Title/MainLogo.png", "Logo") &&
        loadStandaloneSprite("TitleSky", backgroundPath + "Title/Sky_Day.png", "Sky") &&
        loadStandaloneSprite("TitleBackCloud", backgroundPath + "Title/BackCloud.png", "Cloud") &&
        loadStandaloneSprite("TitleMidCloud0", backgroundPath + "Title/MidCloud0.png", "Cloud") &&
        loadStandaloneSprite("TitleMidCloud1", backgroundPath + "Title/MidCloud1.png", "Cloud") &&
        loadStandaloneSprite("TitleFrontCloud", backgroundPath + "Title/FrontCloud.png", "Cloud");
}

bool ResourceManager::loadTrainingVillageResources() {
    const std::string backgroundPath(kBackgroundPath);
    return loadStandaloneSprite("TownSky", backgroundPath + "TownSky.png", "Sky") &&
        loadStandaloneSprite("TownBG", backgroundPath + "TownBG_Day.png", "BG") &&
        loadStandaloneSprite("TownLayer", backgroundPath + "TownLayer_Day.png", "Layer");
}

bool ResourceManager::loadDungeonResources() {
    return loadAtlas("Monster", std::string(kMonsterAtlasJsonPath), std::string(kMonsterAtlasPath)) &&
        loadAtlas("Boss", std::string(kBossAtlasJsonPath), std::string(kBossAtlasPath)) &&
        loadAtlas("Projectile", std::string(kProjectileAtlasJsonPath), std::string(kProjectileAtlasPath)) &&
        loadAtlas("Effect", std::string(kEffectAtlasJsonPath), std::string(kEffectAtlasPath)) &&
        loadAtlas("UI", std::string(kUiAtlasJsonPath), std::string(kUiAtlasPath));
}

const sf::Texture* ResourceManager::getAtlasTexture(const std::string& atlasKey) const {
    const auto atlasIt = m_atlases.find(atlasKey);
    if (atlasIt == m_atlases.end()) {
        std::cerr << "[ResourceManager] 아틀라스를 찾을 수 없습니다: " << atlasKey << '\n';
        return nullptr;
    }
    return &atlasIt->second->texture;
}

const sf::IntRect* ResourceManager::getFrameRect(const std::string& atlasKey,
    const std::string& frameName) const {
    const auto atlasIt = m_atlases.find(atlasKey);
    if (atlasIt == m_atlases.end()) {
        std::cerr << "[ResourceManager] 아틀라스를 찾을 수 없습니다: " << atlasKey << '\n';
        return nullptr;
    }

    const auto frameIt = atlasIt->second->frameRects.find(frameName);
    return frameIt == atlasIt->second->frameRects.end() ? nullptr : &frameIt->second;
}

std::optional<sf::Vector2f> ResourceManager::getFramePivot(const std::string& atlasKey,
    const std::string& frameName) const {
    const auto atlasIt = m_atlases.find(atlasKey);
    if (atlasIt == m_atlases.end()) {
        return std::nullopt;
    }

    const auto pivotIt = atlasIt->second->framePivots.find(frameName);
    return pivotIt == atlasIt->second->framePivots.end()
        ? std::nullopt
        : std::optional<sf::Vector2f>(pivotIt->second);
}

std::optional<sf::Vector2u> ResourceManager::getFrameSourceSize(const std::string& atlasKey,
    const std::string& frameName) const {
    const auto atlasIt = m_atlases.find(atlasKey);
    if (atlasIt == m_atlases.end()) {
        return std::nullopt;
    }

    const auto sourceSizeIt = atlasIt->second->frameSourceSizes.find(frameName);
    return sourceSizeIt == atlasIt->second->frameSourceSizes.end()
        ? std::nullopt
        : std::optional<sf::Vector2u>(sourceSizeIt->second);
}

const std::vector<sf::IntRect>* ResourceManager::getAnimationFrames(
    const std::string& atlasKey, const std::string& animationName) const {
    const auto atlasIt = m_atlases.find(atlasKey);
    if (atlasIt == m_atlases.end()) {
        std::cerr << "[ResourceManager] 아틀라스를 찾을 수 없습니다: " << atlasKey << '\n';
        return nullptr;
    }

    const auto animationIt = atlasIt->second->animations.find(animationName);
    return animationIt == atlasIt->second->animations.end()
        ? nullptr
        : &animationIt->second;
}

std::vector<std::string> ResourceManager::getAnimationNames(const std::string& atlasKey) const {
    std::vector<std::string> names;
    const auto atlasIt = m_atlases.find(atlasKey);
    if (atlasIt == m_atlases.end()) {
        return names;
    }

    names.reserve(atlasIt->second->animations.size());
    for (const auto& [name, frames] : atlasIt->second->animations) {
        names.push_back(name);
    }
    return names;
}

const sf::Font* ResourceManager::getFont(const std::string& fontKey) const {
    const auto fontIt = m_fonts.find(fontKey);
    return fontIt == m_fonts.end() ? nullptr : fontIt->second.get();
}

const sf::Font* ResourceManager::getDefaultFont() const {
    return getFont(std::string(kDefaultFontKey));
}

bool ResourceManager::loadStandaloneSprite(const std::string& atlasKey,
    const std::string& imagePath, const std::string& frameName) {
    auto atlas = std::make_unique<AtlasData>();
    if (!atlas->texture.loadFromFile(imagePath)) {
        std::cerr << "단일 스프라이트 로드 실패: " << imagePath << '\n';
        return false;
    }

    const sf::Vector2u size = atlas->texture.getSize();
    if (size.x == 0 || size.y == 0) {
        std::cerr << "단일 스프라이트 크기가 유효하지 않습니다: " << imagePath << '\n';
        return false;
    }
    const sf::IntRect frameRect({ 0, 0 },
        { static_cast<int>(size.x), static_cast<int>(size.y) });
    atlas->frameRects.emplace(frameName, frameRect);
    atlas->framePivots.emplace(frameName,
        sf::Vector2f{ size.x * 0.5f, size.y * 0.5f });
    atlas->frameSourceSizes.emplace(frameName, size);
    m_atlases[atlasKey] = std::move(atlas);
    return true;
}

std::optional<sf::Vector2f> ResourceManager::getAnimationFirstFramePivot(
    const std::string& atlasKey, const std::string& animationName) const {
    const auto atlasIt = m_atlases.find(atlasKey);
    if (atlasIt == m_atlases.end()) {
        return std::nullopt;
    }
    const auto pivotIt = atlasIt->second->animationPivots.find(animationName);
    if (pivotIt == atlasIt->second->animationPivots.end() || pivotIt->second.empty()) {
        return std::nullopt;
    }
    return pivotIt->second.front();
}
