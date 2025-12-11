#ifndef Game_hpp
#define Game_hpp
#include "SDL.h"
#include "SDL_image.h"
#include <stdio.h>
#include <vector>
#include <utility>

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
    bool isInsideWall(float x, float y);
private:
    bool isRunning;
    SDL_Window *window;
    SDL_Renderer *renderer;
    float playerAngle, FOV=45.0f, playerSpeed=2.0f, rotationSensitivity=0.05f;
    float playerHeight=0.5f, mouseSensitivity=0.002f;
    float playerSquareSize=1.0f;
    std::pair<double, double> playerPosition;
    std::pair<int, int> ScreenHeightWidth;
    std::pair<double, double> playerMoveDirection = {0.0, 0.0};
    std::vector<std::vector<int>> Map, floorMap, ceilingMap;
    std::vector<SDL_Texture*> wallTextures, floorTextures, ceilingTextures;
    std::vector<int> wallTextureWidths, floorTextureWidths, ceilingTextureWidths;
    std::vector<int> wallTextureHeights, floorTextureHeights, ceilingTextureHeights;
};

#endif