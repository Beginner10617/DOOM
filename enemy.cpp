#include "enemy.hpp"
#include <iostream>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <utility>

static float normalizeAngle(float a) {
    while (a <= -M_PI) a += 2.0f * M_PI;
    while (a >   M_PI) a -= 2.0f * M_PI;
    return a;
}

static std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return s;
}

bool parse_enemy_state(const std::string& name, EnemyState& out) {
    std::string n = to_lower(name);

    if (n == "enemy_idle" || n == "idle") {
        out = ENEMY_IDLE;
        return true;
    }
    if (n == "enemy_walk" || n == "walk") {
        out = ENEMY_WALK;
        return true;
    }
    return false;
}

Enemy::Enemy(float x, float y, float theta)
    : position(x, y), angle(theta) {}

std::pair<float, float> Enemy::get_position(){
    return position;
}

float Enemy::get_angle(){
    return angle;
}

void Enemy::_process(float deltaTime) {
    if(state != ENEMY_IDLE){
        fracTime += deltaTime;
        while (fracTime > DurationPerSprite) {
            moveNextFrame();
            fracTime -= DurationPerSprite;
        }
    }
    else{
        fracTime = 0.0f;
        frameIndex = 0;
        currentFrame = Animations[state][0];
    }
    if (walking) {
        float dx = destinationOfWalk.first  - position.first;
        float dy = destinationOfWalk.second - position.second;

        float distSq = dx*dx + dy*dy;
        float step   = moveSpeed * deltaTime;

        if (distSq <= step*step) {
            position = destinationOfWalk;
            walking = false;
            setAnimState(ENEMY_IDLE);
        } else {
            position.first  += step * std::cos(angle);
            position.second -= step * std::sin(angle);
        }
    }
}

void Enemy::addFrame(EnemyState s, int frame) {
    Animations[s].push_back(frame);
}

void Enemy::addFrames(const std::map<EnemyState, std::vector<int>>& Anim) {
    Animations = Anim;
}

void Enemy::setAnimState(EnemyState s){
    state = s;
}

void Enemy::init(){
    addFrames({
        {ENEMY_IDLE, {0}},
        {ENEMY_WALK, {1, 2, 3, 4}},
});
    state = ENEMY_IDLE;
    walkTo(20.0f, 5.0f);
}

void Enemy::updateDirnNumWrt(std::pair<float, float> pos) {
    // Vector from enemy to target
    float dx = pos.first  - position.first;
    float dy = pos.second - position.second;

    // Angle to target (world space)
    float targetAngle = std::atan2(-dy, dx);

    // Relative angle w.r.t enemy facing direction
    float relAngle = normalizeAngle(targetAngle - angle);

    // Each sector is pi/4 wide
    const float sectorSize = M_PI / 4.0f;

    // Shift by pi/8 so that sector 0 is centered at 0
    int dir = static_cast<int>(
        std::floor((relAngle + M_PI / 8.0f) / sectorSize)
    );

    // Wrap to [0, 7]
    if (dir < 0) dir += 8;
    dir %= 8;
    //std::cout<<relAngle*180/M_PI<<" : "<<dir<<std::endl;
    directionNum = dir; 
}

int Enemy::get_current_frame(){
    return currentFrame;
}

int Enemy::get_dirn_num(){
    return directionNum;
}

void Enemy::moveNextFrame() {
    auto &frames = Animations[state];
    if (frames.empty()) return;

    frameIndex = (frameIndex + 1) % frames.size();
    currentFrame = frames[frameIndex];
}

float Enemy::get_size(){
    return sze;
}

void Enemy::walkTo(float x, float y){
    destinationOfWalk = std::make_pair(x, y);
    float dx = x  - position.first;
    float dy = y - position.second;
    angle = std::atan2(-dy, dx);
    setAnimState(ENEMY_WALK);
    walking = true;
}