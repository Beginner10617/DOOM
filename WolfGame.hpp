#ifndef Game_hpp
#define Game_hpp
#include "SDL.h"
#include "SDL_image.h"
#include "enemy.hpp"
#include <stdio.h>
#include <vector>
#include <utility>
#include <map>
using SDLWindowPtr =
    std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)>;

using SDLRendererPtr =
    std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)>;

using SDLTexturePtr =
    std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)>;

class Game{
public:
    Game();
    ~Game() ;
    void init(const char* title, int xpos, int ypos, int width, int height, bool fullscreen);
    void handleEvents();
    void update(float deltaTime);
    void render();
    void clean();
    bool running(){return isRunning;}
    void loadMapDataFromFile(const char* filename);
    void loadColorConfigFromFile(const char* filename);
    void placePlayerAt(int x, int y, float angle);
    void printPlayerPosition();
    void addWallTexture(const char* filePath);
    void addFloorTexture(const char* filePath);
    void addCeilingTexture(const char* filePath);
    bool isDoor(int tileValue);
    bool playerHasKey(int keyType);
    void loadAllTextures(const char* filePath);
    void addEnemy(float x, float y, float angle);
    void loadEnemyTextures(const char* filePath);
    bool collidesWithEnemy(float x, float y);
private:
    bool isRunning;
    SDLWindowPtr   window   {nullptr, SDL_DestroyWindow};
    SDLRendererPtr renderer {nullptr, SDL_DestroyRenderer};
    float playerAngle, FOV=45.0f, playerSpeed=2.0f, rotationSensitivity=0.05f;
    float playerHeight=0.5f, mouseSensitivity=0.002f;
    float playerSquareSize=1.0f;
    std::pair<float, float> playerPosition;
    std::pair<int, int> ScreenHeightWidth;
    std::pair<double, double> playerMoveDirection = {0.0, 0.0};
    std::vector<std::vector<int>> Map, floorMap, ceilingMap;
    std::vector<SDLTexturePtr> wallTextures;
    std::vector<SDLTexturePtr> floorTextures;
    std::vector<SDLTexturePtr> ceilingTextures;
    std::vector<int> wallTextureWidths, floorTextureWidths, ceilingTextureWidths;
    std::vector<int> wallTextureHeights, floorTextureHeights, ceilingTextureHeights;
    struct Door {
        float openAmount;   // 0 = closed, 1 = fully open
        bool opening;       // opening animation active
        bool locked;        // requires key?
        int keyType;        // 0 = none, 1 = blue, 2 = red, 3 = gold
    };

    std::map<std::pair<int,int>, Door> doors;  // key: (mapX,mapY)
    std::vector<int> keysHeld; // keys the player has collected
    std::vector<std::unique_ptr<Enemy>> enemies;
    std::map<std::pair<int, int>, SDLTexturePtr> enemyTextures;

    int health = 100;

    // weapon (current)
    int damageMin = 10;
    int damageMax = 40;
    int accuracyDivisor = 4; // 75% hit
    float fireCooldown = 0.0f;
    float fireRate = 0.2f;

    bool rayCastEnemyToPlayer(
        const Enemy& enemy,
        const std::pair<float,float>& playerPos
    );

};

#endif