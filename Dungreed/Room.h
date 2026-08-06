#pragma once
#include<vector>
enum class RoomType {
    Start,
    Monster,
    Hut,
    Boss

};
struct Door {
    bool isOpen=false;

};
struct RoomInfo {
    std::vector<Door> doors;
    RoomType type;
    bool isClear = false;
    int *tileArray;
};
class Room{
private:
    RoomInfo info;
};

