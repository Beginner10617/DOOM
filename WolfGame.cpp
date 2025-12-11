#include "WolfGame.hpp"
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
        if (!(IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG) & (IMG_INIT_PNG | IMG_INIT_JPG))) {
            SDL_Log("IMG init error: %s", IMG_GetError());
            isRunning = false;
            return;
        }
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

        // Hide cursor and lock on first click
        if (event.type == SDL_MOUSEBUTTONDOWN)
        {
            SDL_ShowCursor(SDL_DISABLE);
            SDL_SetRelativeMouseMode(SDL_TRUE);   // capture mouse
        }

        // Mouse movement → rotate player
        if (event.type == SDL_MOUSEMOTION)
        {
            // event.motion.xrel = delta X since last frame
            playerAngle += event.motion.xrel * mouseSensitivity;
        }
    }

    // Keyboard movement detection
    const Uint8* keystate = SDL_GetKeyboardState(NULL);

    playerMoveDirection = {0.0f, 0.0f};

    // Forward
    if (keystate[SDL_SCANCODE_W] || keystate[SDL_SCANCODE_UP]) {
        playerMoveDirection.first += cos(playerAngle);
        playerMoveDirection.second += sin(playerAngle);
    }

    // Backward
    if (keystate[SDL_SCANCODE_S] || keystate[SDL_SCANCODE_DOWN]) {
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

    // Optional keyboard turning (can keep or remove)
    if (keystate[SDL_SCANCODE_LEFT])
        playerAngle -= rotationSensitivity;

    if (keystate[SDL_SCANCODE_RIGHT])
        playerAngle += rotationSensitivity;

    // Door interaction (Space to open/close)
    if (keystate[SDL_SCANCODE_SPACE])
    {
        int px = (int)(playerPosition.first + playerSquareSize);
        int py = (int)(playerPosition.second);

        // tile directly ahead of player
        int tx = px + (int)round(cos(playerAngle));
        int ty = py + (int)round(sin(playerAngle));

        auto key = std::make_pair(ty, tx);
        if (doors.count(key)) {
            Door &d = doors[key];

            // locked?
            if (d.locked) {
                if (!playerHasKey(d.keyType)) {
                    return;
                }
            }

            // toggle
            if (d.openAmount == 0.0f)
                d.opening = true;
        }
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
    int tileX = Map[(int)playerPosition.second][(int)(newX + playerSquareSize * (newX>playerPosition.first?1:-1))];
    int tileY = Map[(int)(newY + playerSquareSize * (newY>playerPosition.second?1:-1))][(int)playerPosition.first];
    if (tileX == 0 || (isDoor(tileX) && doors[{(int)playerPosition.second, (int)(newX + playerSquareSize * (newX>playerPosition.first?1:-1))}].openAmount > 0.5f))
        playerPosition.first = newX;

    if (tileY == 0 || (isDoor(tileY) && doors[{(int)(newY + playerSquareSize * (newY>playerPosition.second?1:-1)), (int)playerPosition.first}].openAmount > 0.5f))
        playerPosition.second = newY;

    // Update doors
    for (auto &p : doors)
    {
        Door &d = p.second;

        if (d.opening) {
            d.openAmount += 1.5f * deltaTime;
            if (d.openAmount >= 1.0f) {
                d.openAmount = 1.0f;
                d.opening = false;
            }
        }


    }

}

void Game::render()
{
    SDL_SetRenderDrawColor(renderer, 135, 206, 235, 255);
    SDL_RenderClear(renderer);   
    // Draw floor
    if (floorTextures.size() == 0) {
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        SDL_Rect floorRect = {0, ScreenHeightWidth.second / 2, ScreenHeightWidth.first, ScreenHeightWidth.second / 2};
        SDL_RenderFillRect(renderer, &floorRect);
    }

    // Raycasting for walls
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

        // Map tile the ray starts in
        int mapX = (int)playerPosition.first;
        int mapY = (int)playerPosition.second;

        // Length of ray from one x-side to next x-side
        float deltaDistX = (rayDirX == 0) ? 1e30 : fabs(1.0f / rayDirX);
        // Length of ray from one y-side to next y-side
        float deltaDistY = (rayDirY == 0) ? 1e30 : fabs(1.0f / rayDirY);

        int stepX, stepY;
        float sideDistX, sideDistY;

        // Step direction and initial side distances
        if (rayDirX < 0) {
            stepX = -1;
            sideDistX = (playerPosition.first - mapX) * deltaDistX;
        } else {
            stepX = 1;
            sideDistX = (mapX + 1.0f - playerPosition.first) * deltaDistX;
        }

        if (rayDirY < 0) {
            stepY = -1;
            sideDistY = (playerPosition.second - mapY) * deltaDistY;
        } else {
            stepY = 1;
            sideDistY = (mapY + 1.0f - playerPosition.second) * deltaDistY;
        }

        bool hitWall = false;
        int hitSide = 0; // 0 = vertical hit, 1 = horizontal hit

        while (!hitWall)
        {
            // Jump to next grid square
            if (sideDistX < sideDistY) {
                sideDistX += deltaDistX;
                mapX += stepX;
                hitSide = 0;
            } else {
                sideDistY += deltaDistY;
                mapY += stepY;
                hitSide = 1;
            }

            // Check if the ray hit a wall
            // Edit this to include opening doors ---------------------->
            if (Map[mapY][mapX] > 0 && !isDoor(Map[mapY][mapX])) {
                hitWall = true;
            }
            else if (isDoor(Map[mapY][mapX]))
            {
                float open = doors[{mapY, mapX}].openAmount;  // 0..1

                // --- compute hit distance ---
                float hitDist = (hitSide == 0 ? sideDistX - deltaDistX
                                            : sideDistY - deltaDistY);

                // --- exact float hit position ---
                float hitX = playerPosition.first  + rayDirX * hitDist;
                float hitY = playerPosition.second + rayDirY * hitDist;

                // --- local coords inside tile (0..1) ---
                float localX = hitX - floor(hitX);
                float localY = hitY - floor(hitY);

                bool blocks = false;

                if (hitSide == 0) {
                    // vertical → opening affects Y movement
                    // door blocks if hit Y is beyond open amount
                    blocks = (localY >= open);
                } else {
                    // horizontal → opening affects X movement
                    blocks = (localX >= open);
                }

                if (blocks)
                    hitWall = true;      // ray stops here
                else
                    hitWall = false;     // ray passes through (door is open)
            }

        }

        // Distance to wall = distance to side where hit happened
        float distanceToWall;
        if (hitSide == 0)
            distanceToWall = sideDistX - deltaDistX;
        else
            distanceToWall = sideDistY - deltaDistY;

        float hitX = playerPosition.first  + rayDirX * distanceToWall;
        float hitY = playerPosition.second + rayDirY * distanceToWall;
        float wallX;
        if (hitSide == 0)
            wallX = hitY - floor(hitY);
        else
            wallX = hitX - floor(hitX);
        float deltaAngle = rayAngle - playerAngle;
        float correctedDistance = distanceToWall * cos(deltaAngle);

        // Calculate wall height
        int lineHeight = (int)(ScreenHeightWidth.second / correctedDistance);
        int drawStart = -lineHeight / 2 + ScreenHeightWidth.second / 2;
        int drawEnd   =  lineHeight / 2 + ScreenHeightWidth.second / 2;

        if (drawStart < 0) drawStart = 0;
        if (drawEnd >= ScreenHeightWidth.second) drawEnd = ScreenHeightWidth.second - 1;

        // Wall Texture
        int texId = Map[mapY][mapX] - 1;
        int imgWidth = wallTextureWidths[texId], imgHeight = wallTextureHeights[texId];
        SDL_QueryTexture(wallTextures[texId], NULL, NULL, &imgWidth, &imgHeight);
          
        if(!isDoor(texId+1)){
            int texX = (int)(wallX * imgWidth);
            if(hitSide == 0 && rayDirX > 0) texX = imgWidth - texX - 1;
            if(hitSide == 1 && rayDirY < 0) texX = imgWidth - texX - 1;
            if(texX < 0) texX = 0;
            if(texX >= imgWidth) texX = imgWidth - 1; 
            SDL_Rect srcRect = { texX, 0, 1, imgHeight };
            SDL_Rect destRect = { ray, drawStart, 1, drawEnd - drawStart };
            SDL_RenderCopy(renderer, wallTextures[texId], &srcRect, &destRect);
        }
        else if(wallX>doors[{mapY, mapX}].openAmount){
            wallX -= doors[{mapY, mapX}].openAmount;
            int texX = (int)(wallX * imgWidth);
            if(hitSide == 0 && rayDirX > 0) texX = imgWidth - texX - 1;
            if(hitSide == 1 && rayDirY < 0) texX = imgWidth - texX - 1;
            if(texX < 0) texX = 0;
            if(texX >= imgWidth) texX = imgWidth - 1; 
            SDL_Rect srcRect = { texX, 0, 1, imgHeight };
            SDL_Rect destRect = { ray, drawStart, 1, drawEnd - drawStart };
            SDL_RenderCopy(renderer, wallTextures[texId], &srcRect, &destRect);
        }
        // Draw floor texture
        if (floorTextures.size() > 0) {
            int floorScreenStart = drawEnd; // start drawing floor below the wall
            imgHeight = floorTextureHeights[0];
            imgWidth = floorTextureWidths[0];
            for (int y = drawEnd; y < ScreenHeightWidth.second; y++) {
                float rowDist = playerHeight / ((float)y / ScreenHeightWidth.second - 0.5f);

                // Interpolate floor coordinates
                float floorX = playerPosition.first + rowDist * rayDirX;
                float floorY = playerPosition.second + rowDist * rayDirY;

                int texX = ((int)(floorX * imgWidth)) % imgWidth;
                int texY = ((int)(floorY * imgHeight)) % imgHeight;

                SDL_Rect srcRect  = { texX, texY, 1, 1 };
                SDL_Rect destRect = { ray, y, 1, 1 };
                SDL_RenderCopy(renderer, floorTextures[0], &srcRect, &destRect);
            }
        }
        
        // Draw ceiling
        for(int y = 0; y < drawStart; y++) {
            if(ceilingTextures.size() > 0) {
                int imgWidth = ceilingTextureWidths[0];
                int imgHeight = ceilingTextureHeights[0];

                float rowDist = playerHeight / (0.5f - (float)y / ScreenHeightWidth.second);

                // Interpolate ceiling coordinates
                float ceilX = playerPosition.first + rowDist * rayDirX;
                float ceilY = playerPosition.second + rowDist * rayDirY;

                int texX = ((int)(ceilX * imgWidth)) % imgWidth;
                int texY = ((int)(ceilY * imgHeight)) % imgHeight;

                SDL_Rect srcRect  = { texX, texY, 1, 1 };
                SDL_Rect destRect = { ray, y, 1, 1 };
                SDL_RenderCopy(renderer, ceilingTextures[0], &srcRect, &destRect);
            } else {
                SDL_SetRenderDrawColor(renderer, 135, 206, 235, 255); // Sky color
                SDL_RenderDrawPoint(renderer, ray, y);
            }
        }
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
            if (ch >= '6' && ch <= '9') {
                int t = ch - '0';
                Door d;
                d.openAmount = 0.0f;
                d.opening = false;
                if (t == 6) { d.locked = false; d.keyType = 0; }
                if (t == 7) { d.locked = true;  d.keyType = 1; }  // blue key
                if (t == 8) { d.locked = true;  d.keyType = 2; }  // red key
                if (t == 9) { d.locked = true;  d.keyType = 3; }  // gold key

                doors[{Map.size(), row.size()}] = d;
            }
            if (ch >= '0' && ch <= '9') {
                row.push_back(ch - '0');
            }
        }
        Map.push_back(row);
    }
}
void Game::placePlayerAt(int x, int y, float angle) {
    playerPosition = {static_cast<double>(x), static_cast<double>(y)};
    playerAngle = angle;
}
void Game::addWallTexture(const char* filePath) {
    SDL_Texture* texture = IMG_LoadTexture(renderer, filePath);
    if (!texture) {
        std::cerr << "Failed to load texture: " << filePath << " Error: " << IMG_GetError() << std::endl;
        return;
    }
    wallTextures.push_back(texture);
    int width, height;
    SDL_QueryTexture(texture, NULL, NULL, &width, &height);
    wallTextureWidths.push_back(width);
    wallTextureHeights.push_back(height);
}
void Game::addFloorTexture(const char* filePath) {
    SDL_Texture* texture = IMG_LoadTexture(renderer, filePath);
    if (!texture) {
        std::cerr << "Failed to load floor texture: " << filePath << " Error: " << IMG_GetError() << std::endl;
        return;
    }
    floorTextures.push_back(texture);
    int width, height;
    SDL_QueryTexture(texture, NULL, NULL, &width, &height);
    floorTextureWidths.push_back(width);
    floorTextureHeights.push_back(height);
}
void Game::addCeilingTexture(const char* filePath) {
    SDL_Texture* texture = IMG_LoadTexture(renderer, filePath);
    if (!texture) {
        std::cerr << "Failed to load ceiling texture: " << filePath << " Error: " << IMG_GetError() << std::endl;
        return;
    }
    ceilingTextures.push_back(texture);
    int width, height;
    SDL_QueryTexture(texture, NULL, NULL, &width, &height);
    ceilingTextureWidths.push_back(width);
    ceilingTextureHeights.push_back(height);
}
void Game::printPlayerPosition(){
    std::cout << "Player Position: (" << playerPosition.first << ", " << playerPosition.second << ")\n";
}
bool Game::isDoor(int tile) {
    return tile >= 6 && tile <= 9;
}
bool Game::playerHasKey(int keyType) {
    if(keyType == 0) return true; // no key needed
    for (const Door& key : keysHeld) {
        if (key.keyType == keyType) {
            return true;
        }
    }
    return false;
}
void Game::clean()
{
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}