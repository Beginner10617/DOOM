#include "enemy.hpp"
#include <iostream>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <utility>
#include <cstdlib>

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

Enemy::Enemy(float x, float y, float theta)
    : position(x, y), angle(theta) {}

std::pair<float, float> Enemy::get_position() const{
    return position;
}

float Enemy::get_angle() const{
    return angle;
}

void Enemy::_process(float deltaTime, const std::pair<float, float>& playerPosition) {
    updateDirnNumWrt(playerPosition);
    if (!stateLocked)
        thinkTimer += deltaTime;

    if(thinkTimer > thinkInterval){
        thinkTimer = 0.0f;
        think(playerPosition);
    }

    if(state != ENEMY_IDLE){
        fracTime += deltaTime;
        while (fracTime > DurationPerSprite) {
            moveNextFrame();
            fracTime -= DurationPerSprite;
            if(frameIndex == Animations[state].size()-1){
                if(state==ENEMY_DEAD){
                    isDead = true;
                    stateLocked = true;
                    return;
                }
                if(!walking){   // Pain or shooting end
                    stateLocked = false;
                    thinkTimer = 0.0f;
                    fracTime = 0.0f;
                    if(state == ENEMY_SHOOT){
                        damageThisFrame = rollEnemyDamage();
                    }
                }
            }
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
            setAnimState(ENEMY_IDLE, false);
            stateLocked = false;
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

void Enemy::setAnimState(EnemyState s, bool lock = false){
    if (state == s) return;

    state = s;
    stateLocked = lock;
    frameIndex = 0;
    currentFrame = Animations[state][0];
    fracTime = 0.0f;
}

void Enemy::init(){
    addFrames({
        {ENEMY_IDLE, {0}},
        {ENEMY_WALK, {1, 2, 3, 4}},
        {ENEMY_SHOOT, {5, 6, 7}},
        {ENEMY_PAIN, {8, 9}},
        {ENEMY_DEAD, {10, 11, 12, 13}}
});
    setAnimState(ENEMY_IDLE, false);
}

void Enemy::updateDirnNumWrt(const std::pair<float, float>& pos) {
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

int Enemy::get_current_frame() const{
    return currentFrame;
}

int Enemy::get_dirn_num() const{
    return directionNum;
}

void Enemy::moveNextFrame() {
    auto &frames = Animations[state];
    if (frames.empty()) return;

    frameIndex = (frameIndex + 1) % frames.size();
    currentFrame = frames[frameIndex];
}

float Enemy::get_size() const{
    return sze;
}

void Enemy::walkTo(float x, float y){ // for testing purposes
    destinationOfWalk = std::make_pair(x, y);

    float dx = x - position.first;
    float dy = y - position.second;

    float baseAngle = std::atan2(-dy, dx);

    angle = baseAngle ;

    setAnimState(ENEMY_WALK);
    walking = true;
}

void Enemy::think(const std::pair<float, float>& playerPosition){
    if(stateLocked)
        return;
    if(health <= 0 && !isDead){
        setAnimState(ENEMY_DEAD, true);
        return;
    }
    if(justTookDamage && canEnterPain()){
        setAnimState(ENEMY_PAIN, true);
        justTookDamage = false;
        return;
    }
    float dist = std::hypot(
            playerPosition.first  - position.first,
            playerPosition.second - position.second
        );
    bool inAttackRange = (
        dist <= attackRange
    );
    int chanceDivisor = computeEnemyHitChance(dist);
    if(canSeePlayer && inAttackRange && randomAttackChance(chanceDivisor)){
        setAnimState(ENEMY_SHOOT, true);
        // Randomness to decide hit/miss and apply damage to player
        return;
    }
    if(canSeePlayer || alerted){
        if(!walking){
            // Vector to target
            float dx = playerPosition.first - position.first;
            float dy = playerPosition.second - position.second;
            float dist = std::hypot(dx, dy);

            if (dist < 3.0f)
                return;

            // Normalize
            float nx = dx / dist;
            float ny = dy / dist;

            // Random angular error
            float r = static_cast<float>(rand()) / RAND_MAX; // [0,1]
            float error = (r * 2.0f - 1.0f) * walk_angle_error;

            float baseAngle = std::atan2(-ny, nx);
            float finalAngle = baseAngle + error;

            // Choose how far to walk this segment
            float walkDist = std::min(dist, walk_segment_length); 
            
            // Compute deviated destination
            destinationOfWalk.first  = position.first  + walkDist * std::cos(finalAngle);
            destinationOfWalk.second = position.second - walkDist * std::sin(finalAngle);

            angle = std::atan2(
                -(destinationOfWalk.second - position.second),
                (destinationOfWalk.first  - position.first)
            );

            setAnimState(ENEMY_WALK, true);
            walking = true;
        }
        return;
    }
    else{
        setAnimState(ENEMY_IDLE, false);
    }
}


void Enemy::updateCanSeePlayer(bool x){
    canSeePlayer = x;
}
void Enemy::takeDamage(){
    justTookDamage = true;
}

bool Enemy::canEnterPain(){
    if(rand() % painChanceDivisor == 0)
        return true;
    return false;
}

bool Enemy::randomAttackChance(int chanceDivisor){
    if(rand() % chanceDivisor == 0)
        return true;
    return false;
}
int Enemy::computeEnemyHitChance(float dist) {
    const float MIN_DIST = 3.0f;
    const float MAX_DIST = attackRange + 1.0f;

    dist = std::clamp(dist, MIN_DIST, MAX_DIST);
    float t = (dist - MIN_DIST) / (MAX_DIST - MIN_DIST);

    // Quadratic falloff (feels very Wolf-like)
    return (int) attackChanceDivisor * (1.0f - t * t);
}
int Enemy::rollEnemyDamage() {
    return baseDamage + (rand() % damageSpread) - (damageSpread / 2);
}