#pragma once

class Camera;
class Player;
class Room;
class TileMap;

/// 외부에서 생성된 플레이어·카메라를 씬에 공유하고, 맵에 맞는 배치만 담당합니다.
class GameplayContext {
  public:
    GameplayContext(Player &player, Camera &camera);

    bool placePlayerAtRoomSpawn(const Room &room, const TileMap &tileMap, float zoom);
    void setCameraForMap(const TileMap &tileMap, float zoom);

    Player *getPlayer() const { return &m_player; }
    Camera *getCamera() const { return &m_camera; }
    bool isReady() const { return true; }

  private:
    Player &m_player;
    Camera &m_camera;
};
