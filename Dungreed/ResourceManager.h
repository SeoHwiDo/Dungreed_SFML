#pragma once
#include<string>
#include<memory>
#include<optional>

#include<vector>
#include<unordered_map>

#include<SFML/Graphics.hpp>
#include<SFML/Audio.hpp>

//문자열 데이터 직접 소유X->오버헤드 0
\
constexpr std::string_view PLAYER_ATLAS = "Z:/DevProject/Dungreed_Resources/resources/images/player_atlas.png";
constexpr std::string_view MONSTER_ATLAS = "Z:/DevProject/Dungreed_Resources/resources/images/monster_atlas.png";
constexpr std::string_view BOSS_ATLAS = "Z:/DevProject/Dungreed_Resources/resources/images/boss_atlas.png";
constexpr std::string_view TILEMAP_ATLAS = "Z:/DevProject/Dungreed_Resources/resources/images/tilemap_atlas.png";
constexpr std::string_view EQUIP_ATLAS = "Z:/DevProject/Dungreed_Resources/resources/images/equip_atlas.png";
constexpr std::string_view PROJECTILE_ATLAS = "Z:/DevProject/Dungreed_Resources/resources/images/projectile_atlas.png";

constexpr std::string_view PLAYER_JSON = "Z:/DevProject/Dungreed_Resources/resources/images/player_atlas.json";
constexpr std::string_view MONSTER_JSON = "Z:/DevProject/Dungreed_Resources/resources/images/monster_atlas.json";
constexpr std::string_view BOSS_JSON = "Z:/DevProject/Dungreed_Resources/resources/images/boss_atlas.json";
constexpr std::string_view TILEMAP_JSON = "Z:/DevProject/Dungreed_Resources/resources/images/tilemap_atlas.json";
constexpr std::string_view EQUIP_JSON = "Z:/DevProject/Dungreed_Resources/resources/images/equip_atlas.json";
constexpr std::string_view PROJECTILE_JSON = "Z:/DevProject/Dungreed_Resources/resources/images/projectile_atlas.json";
constexpr std::string_view Background_PATH = "Z:/DevProject/Dungreed_Resources/resources/images/Backgrounds/";

struct AtlasData {
    sf::Texture texture;
    // 1. 프레임 풀 네임 -> 사각형 영역 (예: "player_attack_01.png" -> IntRect)
    std::unordered_map<std::string, std::unique_ptr<sf::IntRect>> frameRects;
    // JSON 피벗을 스프라이트 로컬 좌표로 변환해 보관합니다.
    std::unordered_map<std::string, sf::Vector2f> framePivots;
    // JSON sourceSize를 원본 이미지 크기로 보관합니다.
    std::unordered_map<std::string, sf::Vector2u> frameSourceSizes;
    // 2. 애니메이션 클립 이름 -> 프레임 리스트 (예: "player_attack" -> [IntRect0, IntRect1, ...])
    std::unordered_map<std::string, std::unique_ptr<std::vector<sf::IntRect>>> animations;
};
class ResourceManager{
private:
    //태그별 이미지 리소스 관리,모든 리소스 데이터는 리소스매니저만 소유
    std::unordered_map<std::string, std::unique_ptr<AtlasData>> m_atlases;
    ResourceManager() = default;
    ~ResourceManager() = default;
    /// 프레임 파일명에서 숫자 접미사를 제거해 애니메이션 그룹 이름을 만듭니다.
    std::string extractAnimationName(const std::string& frameName) const;


public:

//=============================== Singleton Pattern ==============================
    /// 프로젝트 전체에서 공유하는 유일한 리소스 관리자를 반환합니다. 모든 아틀라스 접근은 이 함수로 시작합니다.
    inline static ResourceManager& getInstance() {
        static ResourceManager instance;
        return instance;
    }

    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;
//=================================================================================
    /// 이미지와 TexturePacker JSON을 읽어 프레임·피벗·원본 크기·애니메이션 목록을 atlasKey로 등록합니다.
    /// 같은 키를 다시 등록하면 기존 데이터가 새 아틀라스 데이터로 교체됩니다.
    bool loadAtlas(const std::string& atlasKey, const std::string& jsonPath, const std::string& imagePath);
    /// 등록된 아틀라스 텍스처의 읽기 전용 포인터를 반환합니다. 키가 없으면 nullptr입니다.
    const sf::Texture* getAtlasTexture(const std::string& atlasKey) const;

    /// 단일 프레임의 텍스처 사각형을 반환합니다. 오브젝트·타일 스프라이트 초기화에 사용하며 없으면 nullptr입니다.
    const sf::IntRect* getFrameRect(const std::string& atlasKey, const std::string& frameName) const;
    /// JSON에 기록된 프레임 피벗을 로컬 좌표로 반환합니다. 피벗 정보가 없으면 nullopt입니다.
    std::optional<sf::Vector2f> getFramePivot(const std::string& atlasKey, const std::string& frameName) const;
    /// 잘리기 전 원본 프레임 크기를 반환합니다. TileMap 셀 크기 결정에 사용하며 없으면 nullopt입니다.
    std::optional<sf::Vector2u> getFrameSourceSize(const std::string& atlasKey, const std::string& frameName) const;

    /// 애니메이션 이름에 묶인 프레임 배열을 반환합니다. Animator 등록용이며 없으면 nullptr입니다.
    const std::vector<sf::IntRect>* getAnimationFrames(const std::string& atlasKey, const std::string& animName) const;
    
    /// 등록 아틀라스에 포함된 모든 애니메이션 이름을 복사해 반환합니다. 초기화 시 일괄 등록에 사용합니다.
    std::vector<std::string> getAnimationNames(const std::string& atlasKey) const;
    
};

