#pragma once
#include<string>
#include<memory>
#include<stdexcept>
#include <filesystem> 
namespace fs = std::filesystem;
#include<unordered_map>
#include<iostream>
#include<SFML/Graphics.hpp>
#include<SFML/Audio.hpp>
const std::string RESOURCE_PATH = "resources/";
const std::string IMAGE_PATH = RESOURCE_PATH + "images/";
const std::string PLAYER_PATH = IMAGE_PATH + "player/";
const std::string MONSTER_PATH = IMAGE_PATH + "monster/";
const std::string BOSS_PATH = IMAGE_PATH + "boss/";
const std::string TILEMAP_PATH = IMAGE_PATH + "TileMaps/";
const std::string Background_PATH = IMAGE_PATH + "Backgrounds/";


class ResourceManager{
private:
	struct ActorImage {
		std::unordered_map<std::string, std::vector<std::unique_ptr<sf::Texture>>> textures;
	};
	ActorImage playerImages;
	std::unordered_map<std::string, ActorImage> enemyImages;
	std::unordered_map<std::string, std::unique_ptr<sf::Texture>> TownTileMapImages;
	std::unordered_map<std::string, std::unique_ptr<sf::Texture>> DungeonTileMapImages;
	bool checkFileExists(const std::string& path) const;
	void loadActorImages(const std::string& path,const std::string&actionName, ActorImage& actorImage);
public:
	// Resource class to manage resources in a game or application
	void loadImages(); // Load resources from files
	const sf::Texture& getPlayerImage(const std::string& action) const;
	const sf::Texture& getEnemyImage(const std::string& name, const std::string& action) const;
	const sf::Texture& getDungeonTileMapImage(const std::string& name, const std::string& type) const;
	const sf::Texture& getTownTileMapImage(const std::string& name, const std::string& type) const;
};

