#pragma once
#include<string>
#include<memory>
#include<stdexcept>
#include<vector>
#include<unordered_map>
#include<iostream>
#include<SFML/Graphics.hpp>
#include<SFML/Audio.hpp>

//문자열 데이터 직접 소유X->오버헤드 0
constexpr std::string_view RESOURCE_PATH = "resources/";
constexpr std::string_view IMAGE_PATH = "resources/images/";

constexpr std::string_view PLAYER_ATLAS = "resources/images/player_atlas.png";
constexpr std::string_view MONSTER_ATLAS = "resources/images/monster_atlas.png";
constexpr std::string_view BOSS_ATLAS = "resources/images/boss_atlas.png";
constexpr std::string_view TILEMAP_ATLAS = "resources/images/TileMaps_atlas.png";

constexpr std::string_view PLAYER_JSON = "resources/images/player_atlas.json";
constexpr std::string_view MONSTER_JSON = "resources/images/monster_atlas.json";
constexpr std::string_view BOSS_JSON = "resources/images/boss_atlas.json";
constexpr std::string_view TILEMAP_JSON = "resources/images/TileMaps_atlas.json";

constexpr std::string_view Background_PATH = "resources/images/Backgrounds/";

struct AtlasData {
    sf::Texture texture;
    // 1. 프레임 풀 네임 -> 사각형 영역 (예: "player_attack_01.png" -> IntRect)
    std::unordered_map<std::string, sf::IntRect> frameRects;
    // 2. 애니메이션 클립 이름 -> 프레임 리스트 (예: "player_attack" -> [IntRect0, IntRect1, ...])
    std::unordered_map<std::string, std::vector<sf::IntRect>> animations;
};
class ResourceManager{
private:
    //태그별 이미지 리소스 관리
    std::unordered_map<std::string, AtlasData> m_atlases;
    ResourceManager() = default;
    ~ResourceManager() = default;
    //애니메이션 클립명과 프레임명 분리
    std::string extractAnimationName(const std::string& frameName) const;


public:

//=============================== Singleton Pattern ==============================
    inline static ResourceManager& getInstance() {
        static ResourceManager instance;
        return instance;
    }

    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;
//=================================================================================
    //아틀라스 로드
    bool loadAtlas(const std::string& atlasKey, const std::string& jsonPath, const std::string& imagePath);
    //아틀라스 이미지 getter
    const sf::Texture& getAtlasTexture(const std::string& atlasKey) const;

    // 2. 특정 단일 프레임의 IntRect 반환 (오브젝트, 타일, 단일 스프라이트용)
    sf::IntRect getFrameRect(const std::string& atlasKey, const std::string& frameName) const;

    // 3. 애니메이션 클립의 전체 프레임 배열 반환 (Actor, Monster의 Animator에 전달용)
    const std::vector<sf::IntRect>& getAnimationFrames(const std::string& atlasKey, const std::string& animName) const;
    
    
    void loadImages(); // Load resources from files
};

