#include "Game.hpp"
#include <iostream>
#include <fstream>
Game::Game(){

}

Game::~Game(){
    
}

void Game::init(const char *title, int xpos, int ypos, int width, int height, bool fullscreen)
{
    if(SDL_Init(SDL_INIT_EVERYTHING) == 0){
        int flags = 0;
        if(fullscreen){
            flags = SDL_WINDOW_FULLSCREEN;
        }
        window = SDL_CreateWindow(title, xpos, ypos, width, height, flags);
        ScreenHeightWidth = std::make_pair(width, height);
        if(window){
            renderer = SDL_CreateRenderer(window, -1, 0);
            if(renderer){
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            }
            isRunning = true;
        }
    } else {
        isRunning = false;
    }
}

void Game::handleEvents()
{
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT)
            isRunning = false;
    }

    // Movement detection
    const Uint8* keystate = SDL_GetKeyboardState(NULL);

    playerMoveDirection = {0.0f, 0.0f};

    // Forward
    if (keystate[SDL_SCANCODE_W]) {
        playerMoveDirection.first += cos(playerAngle);
        playerMoveDirection.second += sin(playerAngle);
    }

    // Backward
    if (keystate[SDL_SCANCODE_S]) {
        playerMoveDirection.first -= cos(playerAngle);
        playerMoveDirection.second -= sin(playerAngle);
    }

    // Strafe Left (A)
    if (keystate[SDL_SCANCODE_A]) {
        playerMoveDirection.first += cos(playerAngle - 3.14159f/2);
        playerMoveDirection.second += sin(playerAngle - 3.14159f/2);
    }

    // Strafe Right (D)
    if (keystate[SDL_SCANCODE_D]) {
        playerMoveDirection.first += cos(playerAngle + 3.14159f/2);
        playerMoveDirection.second += sin(playerAngle + 3.14159f/2);
    }

    // Turn Left
    if (keystate[SDL_SCANCODE_LEFT]) {
        playerAngle -= rotationSensitivity;
    }

    // Turn Right
    if (keystate[SDL_SCANCODE_RIGHT]) {
        playerAngle += rotationSensitivity;
    }
}

void Game::update(float deltaTime)
{
    // Normalize movement direction
    float lengthSquared = playerMoveDirection.first * playerMoveDirection.first +
    playerMoveDirection.second * playerMoveDirection.second;

    if (lengthSquared > 0.0f) {
        float length = sqrt(lengthSquared);
        playerMoveDirection.first /= length;
        playerMoveDirection.second /= length;
    }


    // Collision detection and position update
    float newX = playerPosition.first + playerMoveDirection.first * playerSpeed * deltaTime;
    float newY = playerPosition.second + playerMoveDirection.second * playerSpeed * deltaTime;

    if (Map[(int)playerPosition.second][(int)newX] == 0)
        playerPosition.first = newX;

    if (Map[(int)newY][(int)playerPosition.first] == 0)
        playerPosition.second = newY;

}

void Game::render()
{
    SDL_SetRenderDrawColor(renderer, 135, 206, 235, 255);
    SDL_RenderClear(renderer);   

    int raysCount = ScreenHeightWidth.first;
    float fovRad = FOV * (3.14159f / 180.0f);
    float halfFov = fovRad / 2.0f;

    for (int ray = 0; ray < raysCount; ray++)
    {
        // Angle of this ray
        float rayAngle = playerAngle - halfFov + ray * (fovRad / raysCount);

        // Ray direction
        float rayDirX = cos(rayAngle);
        float rayDirY = sin(rayAngle);

        float distanceToWall = 0.0f;
        const float stepSize = 0.05f;      
        const float maxDepth = 20.0f;

        bool hitWall = false;
        bool boundaryHit = false;

        while (!hitWall && distanceToWall < maxDepth)
        {
            distanceToWall += stepSize;

            int testX = (int)(playerPosition.first + rayDirX * distanceToWall);
            int testY = (int)(playerPosition.second + rayDirY * distanceToWall);

            // Out of bounds is treated as wall
            if (testX < 0 || testX >= Map[0].size() ||
                testY < 0 || testY >= Map.size())
            {
                hitWall = true;
                distanceToWall = maxDepth;
            }
            else
            {
                // Wall hit
                if (Map[testY][testX] > 0)
                {
                    hitWall = true;
                }
            }
        }

        // Remove fisheye distortion
        float delta = rayAngle - playerAngle;
        if (delta > M_PI) delta -= 2 * M_PI;
        if (delta < -M_PI) delta += 2 * M_PI;

        float correctedDistance = distanceToWall * cos(delta);

        // Calculate wall height
        int lineHeight = (int)(ScreenHeightWidth.second / correctedDistance);
        int drawStart = -lineHeight / 2 + ScreenHeightWidth.second / 2;
        int drawEnd   =  lineHeight / 2 + ScreenHeightWidth.second / 2;

        if (drawStart < 0) drawStart = 0;
        if (drawEnd >= ScreenHeightWidth.second) drawEnd = ScreenHeightWidth.second - 1;

        // Wall Shading
        Uint8 shade = (Uint8)(255 * exp(-distanceToWall * 0.15f));
        SDL_SetRenderDrawColor(renderer, shade, shade, shade, 255);
        SDL_RenderDrawLine(renderer, ray, drawStart, ray, drawEnd);
    }

    SDL_RenderPresent(renderer);  
}
void Game::loadMapDataFromFile(const char* filename)
{
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open map data file: " << filename << std::endl;
        return;
    }
    Map.clear();
    std::string line;
    while (std::getline(file, line)) {
        std::vector<int> row;
        for (char& ch : line) {
            if (ch >= '0' && ch <= '9') {
                row.push_back(ch - '0');
            }
        }
        Map.push_back(row);
    }
}
void Game::clean()
{
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}