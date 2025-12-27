#pragma once
#include <map>
#include <string>
#include <utility>
#include <vector>
#define PI 3.1415926535f
// HERE ANGLES ARE TAKEN POSITIVE ANTI-CLOCKWISE FROM TOP CONTRARY TO THE PLAYER
enum EnemyState {
    ENEMY_IDLE,
    ENEMY_WALK,
    ENEMY_SHOOT,
    ENEMY_PAIN,
    ENEMY_DEAD
};

void load_enemy_textures(
    const std::string& filename,
    std::map<std::pair<int, int>, std::string>& Textures
);

class Enemy {
    EnemyState state; 
    bool walking = false;
    float walk_segment_length = 1.5f;

    // perception & memory
    bool alerted = false;
    bool canSeePlayer = false;
    bool justTookDamage = false;
    bool isDead = false;
    bool stateLocked = false;

    // combat stats
    int health = 100;
    int baseDamage = 10;
    int attackChanceDivisor = 2;
    int accuracyDivisor = 6;   // 1 in 6 chance 
    int painChanceDivisor = 4; // 1 in 4 chance
    float walk_angle_error = 10.0f * M_PI / 180.0f; // ±10 degrees
    float attackRange = 7.0f;

    // AI timing
    float thinkTimer = 0.0f;
    float thinkInterval = 0.3f;

    std::pair<float, float> position, destinationOfWalk;
    float angle, sze=1.0f, moveSpeed = 1.0f, DurationPerSprite = 0.25f, fracTime = 0.0f;
    int currentFrame = 0, frameIndex = 0, directionNum;
    std::map<EnemyState, std::vector<int>> Animations;
public:
    Enemy(float x, float y, float theta);
    std::pair<float, float> get_position() const;
    float get_size() const;
    float get_angle() const;
    void _process(float deltaTime, const std::pair<float, float>& pos);
    void addFrame(EnemyState s, int frame);
    void addFrames(const std::map<EnemyState, std::vector<int>>& Anim);
    void setAnimState(EnemyState s, bool );
    void init();
    void updateDirnNumWrt(const std::pair<float, float>& pos);
    int get_current_frame() const;
    int get_dirn_num() const;
    void moveNextFrame();
    void walkTo(float x, float y);

    bool canEnterPain();
    bool randomAttackChance();
    void think(const std::pair<float, float>& pos);
    void updateCanSeePlayer(bool);
    void takeDamage();
};
