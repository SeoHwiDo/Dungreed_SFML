#include "ResourceManager.h"


bool ResourceManager::checkFileExists(const std::string& path) const{
	if(!fs::exists(path)||!fs::is_directory(path)){
		//폴더가 없거나 폴더가 아니면 에러
		std::cerr << "폴더 명이 잘못되었습니다. 경로를 확인해주세요: " << path << std::endl;
		return false;
	}
	return true;
}

void ResourceManager::loadActorImages(const std::string& path, const std::string& actionName, ActorImage& actorImage)
{
	if (checkFileExists(path))return;
	std::vector<std::string>filePath;
	//폴더 내 파일 경로 추출
	for (const auto& entry : fs::directory_iterator(path)) {
		if (fs::is_regular_file(entry.path())) {
			filePath.push_back(entry.path().string());
		}
	}
	std::sort(filePath.begin(), filePath.end());//파일 이름 순으로 정렬

	for (const auto& file : filePath) {
		//확장자
		std::string ext = fs::path(path).extension().string();
		if(ext==".png"||ext==".jpg"||ext==".jpeg"||ext==".bmp"||ext==".tga"||ext==".gif"){
			//이미지 로드
			auto texture = std::make_unique<sf::Texture>();
			if (!texture->loadFromFile(file)) {
				//이미지 로드 실패시 에러 메시지 출력
				std::cerr << "이미지 로드 실패: " << file << std::endl;
				continue;
			}
			//이미지 로드 성공시 textures에 추가
			actorImage.textures[actionName].push_back(std::move(texture));
		}
	}
}

void ResourceManager::loadImages(){
	//플레이어 이미지 로드
	if(checkFileExists(PLAYER_PATH)) {
		for (const auto& entry : fs::directory_iterator(PLAYER_PATH)) {
			//바로 행동 폴더로이동
			if(fs::is_directory(entry.path())){
				//폴더 이름을 액션 이름으로 사용
				std::string actionName = entry.path().filename().string();
				loadActorImages(entry.path().string(), actionName, playerImages);
			}
		}
	}
	//몬스터 이미지 로드
	if (checkFileExists(MONSTER_PATH)) {
		for (const auto& monster : fs::directory_iterator(MONSTER_PATH)) {
			//하위의 몬스터종류별 폴더로 이동
			if (fs::is_directory(monster.path())) {
				for (const auto& entry : fs::directory_iterator(monster.path())) {
					if(fs::is_directory(entry.path())) {
						//폴더 이름을 액션 이름으로 사용
						std::string actionName = entry.path().filename().string();
						loadActorImages(entry.path().string(), actionName, enemyImages[monster.path().filename().string()]);
					}
				}
			}
		}
	}
	//타일맵 이미지 로드

}
