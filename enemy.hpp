#pragma once
#include <map>
#include <string>
#include <utility>
#include <vector>
#define PI 3.1415926535f
// HERE ANGLES ARE TAKEN POSITIVE ANTI-CLOCKWISE FROM TOP CONTRARY TO THE PLAYER
enum EnemyState {
    ENEMY_IDLE,
    ENEMY_WALK
};

bool parse_enemy_state(const std::string& name, EnemyState& out);

void load_enemy_textures(
    const std::string& filename,
    std::map<std::pair<int, int>, std::string>& Textures
);

class Enemy {
    EnemyState state;
    std::pair<float, float> position, destinationOfWalk;
    float angle, sze=1.0f, moveSpeed = 1.0f;
    float DurationPerSprite = 0.25f, fracTime = 0.0f;
    bool walking = false;
    int currentFrame = 0, frameIndex = 0;
    int directionNum;
    std::map<EnemyState, std::vector<int>> Animations;
public:
    Enemy(float x, float y, float theta);
    std::pair<float, float> get_position();
    float get_size();
    float get_angle();
    void _process(float deltaTime);
    void addFrame(EnemyState s, int frame);
    void addFrames(const std::map<EnemyState, std::vector<int>>& Anim);
    void setAnimState(EnemyState s);
    void init();
    void updateDirnNumWrt(std::pair<float, float> pos);
    int get_current_frame();
    int get_dirn_num();
    void moveNextFrame();
    void walkTo(float x, float y);
};
