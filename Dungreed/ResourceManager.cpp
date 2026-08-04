#include "ResourceManager.h"
#include <fstream>
#include <algorithm>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

std::string ResourceManager::extractAnimationName(const std::string& frameName) const
{
    size_t hypenPos = frameName.find_last_of('-');//스트링에서 마지막 언더스코어 위치 찾기, size_t인 이유는 find_last_of는 찾지 못하면 npos를 반환하기 때문
    if (hypenPos != std::string::npos) {
        //프레임 이름에서 마지막 언더스코어까지의 부분 문자열을 반환
        return frameName.substr(0, hypenPos);
    }
    return frameName;
}

bool ResourceManager::loadAtlas(const std::string& atlasKey, const std::string& jsonPath, const std::string& imagePath)
{
    AtlasData atlas;
    //atlas에 텍스쳐 로드, 실패시 에러 메시지 출력 후 false 반환
    if(!atlas.texture.loadFromFile(imagePath)){
        std::cerr << "텍스쳐 로드 실패: " << imagePath << std::endl;
        return false;
    }
    std::ifstream jsonFile(jsonPath);
    if(!jsonFile.is_open()) {
        std::cerr << "JSON 파일 로드 실패: " << jsonPath << std::endl;
        return false;
    }
    json root;
    jsonFile >> root;
    // JSON 구조에서 "frames" 키가 존재하고, 그 값이 객체인지 확인
    if (root.contains("frames") && root["frames"].is_object()) {
        //nlohmann::json은 내부적으로 키를 알파벳 오름차순으로 관리;
        //자리수 패딩시(ex -01,-02) 순차적인 vector 삽입 보장
        for (auto& [frameName, frameData] : root["frames"].items()) {
            //현재 프레임의 이름과 데이터를 가져옴
            auto& f = frameData["frame"];
            sf::IntRect rect(
                { f["x"].get<int>(), f["y"].get<int>() },
                { f["w"].get<int>(), f["h"].get<int>() }
            );
            //프레임 이름과 IntRect 저장
            atlas.frameRects[frameName] = rect;
            //프레임 애니메이션 저장
            std::string animName = extractAnimationName(frameName);
            atlas.animations[animName].push_back(rect);
        }
    }
    //최종 관리책임 이양
    m_atlases[atlasKey] = std::move(atlas);
    return true;
}

const sf::Texture& ResourceManager::getAtlasTexture(const std::string& atlasKey) const
{
    //아틀라스 존재하는지 확인
    auto it = m_atlases.find(atlasKey);
    if (it == m_atlases.end()) {
        std::cerr << "아틀라스 키를 찾을 수 없음:" << atlasKey << std::endl;
        return;
    }
    return it->second.texture;
}
//try-catch는 stack 언와인딩 동반하므로 연산이 비쌈
//단일 리소스 누락의 사소한 건으로 게임 전체를 정지시키는것은 X
sf::IntRect ResourceManager::getFrameRect(const std::string& atlasKey, const std::string& frameName) const
{
    //atlaskey 없으면 out_of_range. 아틀라스 자체가 없으면 게임 속행이 불가
    const auto& atlas = m_atlases.at(atlasKey);
    //단순한 객체 하나정도 없는경우는 게임 속행
    auto it = atlas.frameRects.find(frameName);
    if (it != atlas.frameRects.end()) {
        return it->second;
    }
    return sf::IntRect({ 0, 0 }, { 0, 0 });
}

const std::vector<sf::IntRect>& ResourceManager::getAnimationFrames(const std::string& atlasKey, const std::string& animName) const
{
    const auto& atlas = m_atlases.at(atlasKey);
    auto it = atlas.animations.find(animName);
    if (it != atlas.animations.end()) {
        return it->second;
    }
    static const std::vector<sf::IntRect> emptyVec;
    return emptyVec;
}


