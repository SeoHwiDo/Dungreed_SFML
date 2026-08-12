#pragma once

#include <SFML/Audio.hpp>
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
constexpr std::string_view kEffectAtlasPath = "resources/images/effect_atlas.png";
constexpr std::string_view kUiAtlasPath = "resources/images/ui_atlas.png";
constexpr std::string_view kDefaultFontPath = "resources/Font/french.ttf";
constexpr std::string_view kDefaultFontKey = "Default";

constexpr std::string_view kPlayerAtlasJsonPath = "resources/images/player_atlas.json";
constexpr std::string_view kMonsterAtlasJsonPath = "resources/images/monster_atlas.json";
constexpr std::string_view kBossAtlasJsonPath = "resources/images/boss_atlas.json";
constexpr std::string_view kTileMapAtlasJsonPath = "resources/images/tilemap_atlas.json";
constexpr std::string_view kEquipAtlasJsonPath = "resources/images/equip_atlas.json";
constexpr std::string_view kProjectileAtlasJsonPath = "resources/images/projectile_atlas.json";
constexpr std::string_view kEffectAtlasJsonPath = "resources/images/effect_atlas.json";
constexpr std::string_view kUiAtlasJsonPath = "resources/images/ui_atlas.json";

constexpr std::string_view kBackgroundPath = "resources/images/Backgrounds/";

struct AtlasData {
    sf::Texture texture;
    std::unordered_map<std::string, sf::IntRect> frameRects;
    std::unordered_map<std::string, sf::Vector2f> framePivots;
    std::unordered_map<std::string, std::vector<sf::Vector2f>> animationPivots;
    std::unordered_map<std::string, sf::Vector2u> frameSourceSizes;
    std::unordered_map<std::string, std::vector<sf::IntRect>> animations;
};

class ResourceManager {
  public:
    static ResourceManager &getInstance() {
        static ResourceManager instance;
        return instance;
    }

    ResourceManager(const ResourceManager &) = delete;
    ResourceManager &operator=(const ResourceManager &) = delete;

    // 그래픽 리소스 로드
    bool loadAtlas(const std::string &atlasKey, const std::string &jsonPath, const std::string &imagePath);
    bool loadStandaloneSprite(const std::string &atlasKey, const std::string &imagePath, const std::string &frameName);

    // 폰트 리소스 로드
    bool loadFont(const std::string &fontKey, const std::string &fontPath);
    bool loadDefaultFont(const std::string &fontPath);

    // 장면별 리소스 묶음 로드
    /// main은 개별 파일 경로를 알 필요가 없습니다.
    bool loadSharedGameplayResources();
    bool loadSharedAudioResources();
    bool loadTitleResources();
    bool loadTrainingVillageResources();
    bool loadDungeonResources();

    // 그래픽 리소스 조회
    /// JSON 아틀라스가 없는 단일 PNG를 프레임 하나짜리 아틀라스로 등록합니다.
    const sf::Texture *getAtlasTexture(const std::string &atlasKey) const;
    const sf::IntRect *getFrameRect(const std::string &atlasKey, const std::string &frameName) const;
    std::optional<sf::Vector2f> getFramePivot(const std::string &atlasKey, const std::string &frameName) const;
    std::optional<sf::Vector2u> getFrameSourceSize(const std::string &atlasKey, const std::string &frameName) const;
    const std::vector<sf::IntRect> *getAnimationFrames(const std::string &atlasKey, const std::string &animationName) const;
    /// 애니메이션 첫 프레임의 피벗을 반환합니다. 장식처럼 별도 스프라이트를 배치할 때 사용합니다.
    std::optional<sf::Vector2f> getAnimationFirstFramePivot(const std::string &atlasKey, const std::string &animationName) const;
    std::vector<std::string> getAnimationNames(const std::string &atlasKey) const;

    // 폰트 리소스 조회
    const sf::Font *getFont(const std::string &fontKey) const;
    const sf::Font *getDefaultFont() const;

    // 오디오 리소스 조회: 확장자를 제외한 WAV 파일명으로 가져옵니다.
    const std::vector<sf::SoundBuffer> *getSoundBuffers(const std::string &soundName) const;
    const sf::SoundBuffer *getSfxTemplate() const;
    const std::string *getMusicPath(const std::string &musicName) const;
    std::vector<std::string> getMusicNames() const;

  private:
    ResourceManager() = default;
    ~ResourceManager() = default;

    // 내부 파싱 보조 함수
    bool loadAudioDirectory(const std::string &audioDirectory);
    std::string extractAudioName(const std::string &fileName) const;
    std::string extractAnimationName(const std::string &frameName) const;

    std::unordered_map<std::string, std::unique_ptr<AtlasData>> m_atlases;
    std::unordered_map<std::string, std::unique_ptr<sf::Font>> m_fonts;
    std::unordered_map<std::string, std::vector<sf::SoundBuffer>> m_soundBuffers;
    std::unordered_map<std::string, std::string> m_musicPaths;
    bool m_audioResourcesLoaded = false;
};
