#include "ResourceManager.h"

#include "LogManager.h"

#include <filesystem>
#include <fstream>
#include <iostream>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

// -----------------------------------------------------------------------------
// 그래픽 리소스 로드
// -----------------------------------------------------------------------------

bool ResourceManager::loadAtlas(const std::string &atlasKey, const std::string &jsonPath, const std::string &imagePath) {
    // Sprite와 Animator는 아틀라스 내부 텍스처·프레임 배열을 참조합니다.
    // 장면 재진입 시 같은 키를 교체하면 살아 있는 풀 객체의 참조가 무효화되므로,
    // 이미 로드한 아틀라스는 재사용합니다.
    if (m_atlases.find(atlasKey) != m_atlases.end()) {
        return true;
    }

    auto atlas = std::make_unique<AtlasData>();
    if (!atlas->texture.loadFromFile(imagePath)) {
        LogManager::getInstance().error("ResourceManager", "텍스처 로드 실패: " + imagePath);
        return false;
    }

    std::ifstream jsonFile(jsonPath);
    if (!jsonFile) {
        LogManager::getInstance().error("ResourceManager", "아틀라스 JSON 파일 로드 실패: " + jsonPath);
        return false;
    }

    json root;
    try {
        jsonFile >> root;
    } catch (const json::exception &exception) {
        LogManager::getInstance().error("ResourceManager", "Failed to parse atlas JSON: " + jsonPath + " (" + exception.what() + ')');
        return false;
    }
    if (!root.contains("frames") || !root["frames"].is_object()) {
        LogManager::getInstance().error("ResourceManager", "아틀라스 frames 데이터가 없습니다: " + jsonPath);
        return false;
    }

    for (const auto &[frameName, frameData] : root["frames"].items()) {
        const auto &frame = frameData.at("frame");
        const sf::IntRect rect{{frame.at("x").get<int>(), frame.at("y").get<int>()}, {frame.at("w").get<int>(), frame.at("h").get<int>()}};
        atlas->frameRects.emplace(frameName, rect);

        sf::Vector2f sourceSize{static_cast<float>(rect.size.x), static_cast<float>(rect.size.y)};
        if (frameData.contains("sourceSize")) {
            const auto &sourceSizeData = frameData.at("sourceSize");
            const sf::Vector2u sourceSizePixels{sourceSizeData.at("w").get<unsigned int>(), sourceSizeData.at("h").get<unsigned int>()};
            atlas->frameSourceSizes.emplace(frameName, sourceSizePixels);
            sourceSize = {static_cast<float>(sourceSizePixels.x), static_cast<float>(sourceSizePixels.y)};
        }

        sf::Vector2f pivot{rect.size.x / 2.f, rect.size.y / 2.f};
        if (frameData.contains("pivot") && frameData.contains("spriteSourceSize")) {
            const auto &pivotData = frameData.at("pivot");
            const auto &spriteSourceSize = frameData.at("spriteSourceSize");
            pivot = {pivotData.at("x").get<float>() * sourceSize.x - spriteSourceSize.at("x").get<float>(), pivotData.at("y").get<float>() * sourceSize.y - spriteSourceSize.at("y").get<float>()};
        }
        atlas->framePivots.emplace(frameName, pivot);

        const std::string animationName = extractAnimationName(frameName);
        atlas->animations[animationName].push_back(rect);
        atlas->animationPivots[animationName].push_back(pivot);
    }

    m_atlases[atlasKey] = std::move(atlas);
    return true;
}

bool ResourceManager::loadStandaloneSprite(const std::string &atlasKey, const std::string &imagePath, const std::string &frameName) {
    auto atlas = std::make_unique<AtlasData>();
    if (!atlas->texture.loadFromFile(imagePath)) {
        LogManager::getInstance().error("ResourceManager", "단일 스프라이트 로드 실패: " + imagePath);
        return false;
    }

    const sf::Vector2u size = atlas->texture.getSize();
    if (size.x == 0 || size.y == 0) {
        LogManager::getInstance().error("ResourceManager", "단일 스프라이트 크기가 유효하지 않습니다: " + imagePath);
        return false;
    }
    const sf::IntRect frameRect({0, 0}, {static_cast<int>(size.x), static_cast<int>(size.y)});
    atlas->frameRects.emplace(frameName, frameRect);
    atlas->framePivots.emplace(frameName, sf::Vector2f{size.x * 0.5f, size.y * 0.5f});
    atlas->frameSourceSizes.emplace(frameName, size);
    m_atlases[atlasKey] = std::move(atlas);
    return true;
}

// -----------------------------------------------------------------------------
// 폰트 리소스 로드
// -----------------------------------------------------------------------------

bool ResourceManager::loadFont(const std::string &fontKey, const std::string &fontPath) {
    auto font = std::make_unique<sf::Font>();
    if (!font->openFromFile(fontPath)) {
        LogManager::getInstance().error("ResourceManager", "폰트 로드 실패: " + fontPath);
        return false;
    }
    m_fonts[fontKey] = std::move(font);
    return true;
}

bool ResourceManager::loadDefaultFont(const std::string &fontPath) { return loadFont(std::string(kDefaultFontKey), fontPath); }

// -----------------------------------------------------------------------------
// 오디오 리소스 로드
// -----------------------------------------------------------------------------

bool ResourceManager::loadAudioDirectory(const std::string &audioDirectory) {
    const std::filesystem::path root(audioDirectory);
    const std::filesystem::path sfxDirectory = root / "SFX";
    const std::filesystem::path bgmDirectory = root / "BGM";
    if (!std::filesystem::is_directory(sfxDirectory) || !std::filesystem::is_directory(bgmDirectory)) {
        LogManager::getInstance().error("ResourceManager", "Audio SFX/BGM directory was not found: " + root.string());
        return false;
    }

    std::unordered_map<std::string, std::vector<sf::SoundBuffer>> soundBuffers;
    for (const std::filesystem::directory_entry &entry : std::filesystem::directory_iterator(sfxDirectory)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".wav")
            continue;
        sf::SoundBuffer sound;
        if (!sound.loadFromFile(entry.path().string())) {
            LogManager::getInstance().error("ResourceManager", "SFX file could not be loaded: " + entry.path().string());
            return false;
        }
        soundBuffers[extractAudioName(entry.path().stem().string())].push_back(std::move(sound));
    }

    std::unordered_map<std::string, std::string> musicPaths;
    for (const std::filesystem::directory_entry &entry : std::filesystem::directory_iterator(bgmDirectory)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".wav")
            continue;
        musicPaths.emplace(entry.path().stem().string(), entry.path().string());
    }
    if (soundBuffers.empty() || musicPaths.empty()) {
        LogManager::getInstance().error("ResourceManager", "Audio directory has no usable WAV files: " + root.string());
        return false;
    }

    m_soundBuffers = std::move(soundBuffers);
    m_musicPaths = std::move(musicPaths);
    return true;
}

// -----------------------------------------------------------------------------
// 장면별 리소스 묶음 로드
// -----------------------------------------------------------------------------

bool ResourceManager::loadSharedGameplayResources() { return loadAtlas("Player", std::string(kPlayerAtlasJsonPath), std::string(kPlayerAtlasPath)) && loadAtlas("TileMap", std::string(kTileMapAtlasJsonPath), std::string(kTileMapAtlasPath)) && loadAtlas("Equip", std::string(kEquipAtlasJsonPath), std::string(kEquipAtlasPath)); }

bool ResourceManager::loadSharedAudioResources() {
    if (m_audioResourcesLoaded) {
        return true;
    }
    m_audioResourcesLoaded = loadAudioDirectory("resources/Audios");
    return m_audioResourcesLoaded;
}

bool ResourceManager::loadTitleResources() {
    const std::string backgroundPath(kBackgroundPath);
    return loadDefaultFont(std::string(kDefaultFontPath)) && loadStandaloneSprite("TitleLogo", backgroundPath + "Title/MainLogo.png", "Logo") && loadStandaloneSprite("TitleSky", backgroundPath + "Title/Sky_Day.png", "Sky") && loadStandaloneSprite("TitleBackCloud", backgroundPath + "Title/BackCloud.png", "Cloud") && loadStandaloneSprite("TitleMidCloud0", backgroundPath + "Title/MidCloud0.png", "Cloud") && loadStandaloneSprite("TitleMidCloud1", backgroundPath + "Title/MidCloud1.png", "Cloud") && loadStandaloneSprite("TitleFrontCloud", backgroundPath + "Title/FrontCloud.png", "Cloud");
}

bool ResourceManager::loadTrainingVillageResources() {
    const std::string backgroundPath(kBackgroundPath);
    return loadStandaloneSprite("TownSky", backgroundPath + "TownSky.png", "Sky") && loadStandaloneSprite("TownBG", backgroundPath + "TownBG_Day.png", "BG") && loadStandaloneSprite("TownLayer", backgroundPath + "TownLayer_Day.png", "Layer");
}

bool ResourceManager::loadDungeonResources() { return loadAtlas("Monster", std::string(kMonsterAtlasJsonPath), std::string(kMonsterAtlasPath)) && loadAtlas("Boss", std::string(kBossAtlasJsonPath), std::string(kBossAtlasPath)) && loadAtlas("Projectile", std::string(kProjectileAtlasJsonPath), std::string(kProjectileAtlasPath)) && loadAtlas("Effect", std::string(kEffectAtlasJsonPath), std::string(kEffectAtlasPath)) && loadAtlas("UI", std::string(kUiAtlasJsonPath), std::string(kUiAtlasPath)); }

// -----------------------------------------------------------------------------
// 그래픽 리소스 조회
// -----------------------------------------------------------------------------

const sf::Texture *ResourceManager::getAtlasTexture(const std::string &atlasKey) const {
    const auto atlasIt = m_atlases.find(atlasKey);
    if (atlasIt == m_atlases.end()) {
        LogManager::getInstance().error("ResourceManager", "아틀라스를 찾을 수 없습니다: " + atlasKey);
        return nullptr;
    }
    return &atlasIt->second->texture;
}

const sf::IntRect *ResourceManager::getFrameRect(const std::string &atlasKey, const std::string &frameName) const {
    const auto atlasIt = m_atlases.find(atlasKey);
    if (atlasIt == m_atlases.end()) {
        LogManager::getInstance().error("ResourceManager", "아틀라스를 찾을 수 없습니다: " + atlasKey);
        return nullptr;
    }

    const auto frameIt = atlasIt->second->frameRects.find(frameName);
    if (frameIt == atlasIt->second->frameRects.end()) {
        LogManager::getInstance().error("ResourceManager", "Frame was not found: " + atlasKey + "/" + frameName);
        return nullptr;
    }
    return &frameIt->second;
}

std::optional<sf::Vector2f> ResourceManager::getFramePivot(const std::string &atlasKey, const std::string &frameName) const {
    const auto atlasIt = m_atlases.find(atlasKey);
    if (atlasIt == m_atlases.end()) {
        LogManager::getInstance().error("ResourceManager", "Atlas was not found while requesting a frame pivot: " + atlasKey);
        return std::nullopt;
    }

    const auto pivotIt = atlasIt->second->framePivots.find(frameName);
    if (pivotIt == atlasIt->second->framePivots.end()) {
        LogManager::getInstance().error("ResourceManager", "Frame pivot was not found: " + atlasKey + "/" + frameName);
        return std::nullopt;
    }
    return pivotIt->second;
}

std::optional<sf::Vector2u> ResourceManager::getFrameSourceSize(const std::string &atlasKey, const std::string &frameName) const {
    const auto atlasIt = m_atlases.find(atlasKey);
    if (atlasIt == m_atlases.end()) {
        LogManager::getInstance().error("ResourceManager", "아틀라스를 찾을 수 없습니다: " + atlasKey);
        return std::nullopt;
    }

    const auto sourceSizeIt = atlasIt->second->frameSourceSizes.find(frameName);
    if (sourceSizeIt == atlasIt->second->frameSourceSizes.end()) {
        LogManager::getInstance().error("ResourceManager", "Frame source size was not found: " + atlasKey + "/" + frameName);
        return std::nullopt;
    }
    return sourceSizeIt->second;
}

const std::vector<sf::IntRect> *ResourceManager::getAnimationFrames(const std::string &atlasKey, const std::string &animationName) const {
    const auto atlasIt = m_atlases.find(atlasKey);
    if (atlasIt == m_atlases.end()) {
        LogManager::getInstance().error("ResourceManager", "아틀라스를 찾을 수 없습니다: " + atlasKey);
        return nullptr;
    }

    const auto animationIt = atlasIt->second->animations.find(animationName);
    if (animationIt == atlasIt->second->animations.end()) {
        LogManager::getInstance().error("ResourceManager", "Animation was not found: " + atlasKey + "/" + animationName);
        return nullptr;
    }
    return &animationIt->second;
}

std::optional<sf::Vector2f> ResourceManager::getAnimationFirstFramePivot(const std::string &atlasKey, const std::string &animationName) const {
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

std::vector<std::string> ResourceManager::getAnimationNames(const std::string &atlasKey) const {
    std::vector<std::string> names;
    const auto atlasIt = m_atlases.find(atlasKey);
    if (atlasIt == m_atlases.end()) {
        LogManager::getInstance().error("ResourceManager", "Animation list requested from an unavailable atlas: " + atlasKey);
        return names;
    }

    names.reserve(atlasIt->second->animations.size());
    for (const auto &[name, frames] : atlasIt->second->animations) {
        names.push_back(name);
    }
    return names;
}

// -----------------------------------------------------------------------------
// 폰트 리소스 조회
// -----------------------------------------------------------------------------

const sf::Font *ResourceManager::getFont(const std::string &fontKey) const {
    const auto fontIt = m_fonts.find(fontKey);
    if (fontIt == m_fonts.end()) {
        LogManager::getInstance().error("ResourceManager", "Font was not found: " + fontKey);
        return nullptr;
    }
    return fontIt->second.get();
}

const sf::Font *ResourceManager::getDefaultFont() const { return getFont(std::string(kDefaultFontKey)); }

// -----------------------------------------------------------------------------
// 오디오 리소스 조회
// -----------------------------------------------------------------------------

const std::vector<sf::SoundBuffer> *ResourceManager::getSoundBuffers(const std::string &soundName) const {
    const auto soundIt = m_soundBuffers.find(soundName);
    if (soundIt == m_soundBuffers.end()) {
        LogManager::getInstance().warning("ResourceManager", "Sound was not found: " + soundName);
        return nullptr;
    }
    return &soundIt->second;
}

const sf::SoundBuffer *ResourceManager::getSfxTemplate() const {
    for (const auto &[soundName, sounds] : m_soundBuffers) {
        if (!sounds.empty()) {
            return &sounds.front();
        }
    }
    return nullptr;
}

const std::string *ResourceManager::getMusicPath(const std::string &musicName) const {
    const auto musicIt = m_musicPaths.find(musicName);
    if (musicIt == m_musicPaths.end()) {
        LogManager::getInstance().warning("ResourceManager", "Music was not found: " + musicName);
        return nullptr;
    }
    return &musicIt->second;
}

std::vector<std::string> ResourceManager::getMusicNames() const {
    std::vector<std::string> names;
    names.reserve(m_musicPaths.size());
    for (const auto &[name, path] : m_musicPaths)
        names.push_back(name);
    return names;
}

// -----------------------------------------------------------------------------
// 내부 파싱 보조 함수
// -----------------------------------------------------------------------------

std::string ResourceManager::extractAnimationName(const std::string &frameName) const {
    const std::size_t slashPosition = frameName.find_last_of('/');
    const std::string name = slashPosition == std::string::npos ? frameName : frameName.substr(slashPosition + 1);

    const std::size_t hyphenPosition = name.find_last_of('-');
    return hyphenPosition == std::string::npos ? name : name.substr(0, hyphenPosition);
}

std::string ResourceManager::extractAudioName(const std::string &fileName) const {
    const std::size_t variantPosition = fileName.find_last_of('-');
    return variantPosition == std::string::npos ? fileName : fileName.substr(0, variantPosition);
}
