#include "WolfGame.hpp"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>
Game::Game(){

}

Game::~Game(){
    clean();
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
        window.reset(SDL_CreateWindow(title, xpos, ypos, width, height, flags));
        ScreenHeightWidth = std::make_pair(width, height);
        if(window){
            renderer.reset(SDL_CreateRenderer(window.get(), -1, 0));
            if(renderer.get()){
                SDL_SetRenderDrawColor(renderer.get(), 255, 255, 255, 255);
            }
            isRunning = true;
        }
    } else {
        isRunning = false;
    }
    for(const std::unique_ptr<Enemy>& e : enemies){
        e->init();
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
            playerAngle = fmod(playerAngle, 2 * PI);
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
    // Door interaction (Space to open/close)
    if (keystate[SDL_SCANCODE_SPACE])
    {
        int tx = (int)(playerPosition.first  + cos(playerAngle) * playerSquareSize *1.1f);
        int ty = (int)(playerPosition.second + sin(playerAngle) * playerSquareSize *1.1f);

        auto key = std::make_pair(ty, tx);
        if (doors.count(key)) {
            Door& d = doors[key];

            if (d.locked && !playerHasKey(d.keyType))
                return;

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
    int mx = (int)newX;
    int my = (int)playerPosition.second;
    if (my < 0 || my >= Map.size()) return;
    if (mx < 0 || mx >= Map[my].size()) return;

    int tileX = Map[(int)playerPosition.second][(int)(newX + playerSquareSize * (newX>playerPosition.first?1:-1))];
    int tileY = Map[(int)(newY + playerSquareSize * (newY>playerPosition.second?1:-1))][(int)playerPosition.first];
    if (tileX == 0 ||
        (isDoor(tileX) &&
        doors[{(int)playerPosition.second,
                (int)(newX + playerSquareSize * (newX > playerPosition.first ? 1 : -1))}]
            .openAmount > 0.5f))
    {
        if (!collidesWithEnemy(newX, playerPosition.second)) {
            playerPosition.first = newX;
        }
    }

    if (tileY == 0 ||
        (isDoor(tileY) &&
        doors[{(int)(newY + playerSquareSize * (newY > playerPosition.second ? 1 : -1)),
                (int)playerPosition.first}]
            .openAmount > 0.5f))
    {
        if (!collidesWithEnemy(playerPosition.first, newY)) {
            playerPosition.second = newY;
        }
    }

    // Update doors
    for (auto& [pos, d] : doors)
    {
        if (d.opening) {
            d.openAmount += 1.5f * deltaTime;
            if (d.openAmount >= 1.0f) {
                d.openAmount = 1.0f;
                d.opening = false;
            }
        }
    }


    // Update enemies
    for(const std::unique_ptr<Enemy>& e : enemies){
        e->_process(deltaTime, playerPosition);
        // Update canSeePlayer
        e->updateCanSeePlayer(
            rayCastEnemyToPlayer(*e, playerPosition)
        );
    }
}

void Game::render()
{
    SDL_SetRenderDrawColor(renderer.get(), 40, 40, 40, 255);
    SDL_RenderClear(renderer.get());   
    std::vector<float> zBuffer(ScreenHeightWidth.first);

    // Draw floor
    if (floorTextures.size() == 0) {
        SDL_SetRenderDrawColor(renderer.get(), 100, 100, 100, 255);
        SDL_Rect floorRect = {0, ScreenHeightWidth.second / 2, ScreenHeightWidth.first, ScreenHeightWidth.second / 2};
        SDL_RenderFillRect(renderer.get(), &floorRect);
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
            if (Map[mapY][mapX] > 0 && !isDoor(Map[mapY][mapX])) {
                hitWall = true;
            }
            else if (isDoor(Map[mapY][mapX]))
            {
                float open = 0.0f;
                auto it = doors.find({mapY, mapX});
                if (it != doors.end()) {
                    open = it->second.openAmount;
                } else {
                    std::cout << "Door at "<<mapY<<", "<<mapX<<" not found\n";
                    return;
                }
                
                // --- compute hit distance ---
                float hitDist = (hitSide == 0 ? sideDistX - deltaDistX
                                            : sideDistY - deltaDistY);

                // exact float hit position
                float hitX = playerPosition.first  + rayDirX * hitDist;
                float hitY = playerPosition.second + rayDirY * hitDist;

                // local coords inside tile (0..1)
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
        zBuffer[ray] = correctedDistance;

        // Calculate wall height
        int lineHeight = (int)(ScreenHeightWidth.second / correctedDistance);
        int drawStart = -lineHeight / 2 + ScreenHeightWidth.second / 2;
        int drawEnd   =  lineHeight / 2 + ScreenHeightWidth.second / 2;

        if (drawStart < 0) drawStart = 0;
        if (drawEnd >= ScreenHeightWidth.second) drawEnd = ScreenHeightWidth.second - 1;

        // Wall Texture
        int texId = Map[mapY][mapX] - 1;
        int imgWidth = wallTextureWidths[texId], imgHeight = wallTextureHeights[texId];
        SDL_QueryTexture(wallTextures[texId].get(), NULL, NULL, &imgWidth, &imgHeight);

        // -------- distance-based shading --------
        float maxLightDist = 8.0f;
        float shade = 1.0f - std::min(correctedDistance / maxLightDist, 1.0f);
        Uint8 brightness = (Uint8)(40 + shade * 215);
        SDL_SetTextureColorMod(wallTextures[texId].get(),
                            brightness, brightness, brightness);
        // --------------------------------------

        if(!isDoor(texId+1)){
            int texX = (int)(wallX * imgWidth);
            if(hitSide == 0 && rayDirX > 0) texX = imgWidth - texX - 1;
            if(hitSide == 1 && rayDirY < 0) texX = imgWidth - texX - 1;
            texX = std::clamp(texX, 0, imgWidth - 1);

            SDL_Rect srcRect  = { texX, 0, 1, imgHeight };
            SDL_Rect destRect = { ray, drawStart, 1, drawEnd - drawStart };
            SDL_RenderCopy(renderer.get(), wallTextures[texId].get(), &srcRect, &destRect);
        }
        else if (wallX > doors[{mapY, mapX}].openAmount) {

            wallX -= doors[{mapY, mapX}].openAmount;
            int texX = (int)(wallX * imgWidth);
            if(hitSide == 0 && rayDirX > 0) texX = imgWidth - texX - 1;
            if(hitSide == 1 && rayDirY < 0) texX = imgWidth - texX - 1;
            texX = std::clamp(texX, 0, imgWidth - 1);

            SDL_Rect srcRect  = { texX, 0, 1, imgHeight };
            SDL_Rect destRect = { ray, drawStart, 1, drawEnd - drawStart };
            SDL_RenderCopy(renderer.get(), wallTextures[texId].get(), &srcRect, &destRect);
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
                SDL_RenderCopy(renderer.get(), floorTextures[0].get(), &srcRect, &destRect);
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
                SDL_RenderCopy(renderer.get(), ceilingTextures[0].get(), &srcRect, &destRect);
            } 
        }
    }
    // Rendering Enemy
    // SORT the enemies list before here (TBD)
    for (const auto& enemy : enemies) 
    {
        // Enemy position relative to player 
        auto [ex, ey] = enemy->get_position();

        float dx = ex - playerPosition.first;
        float dy = ey - playerPosition.second;

        float enemyDist = sqrt(dx*dx + dy*dy);

        // Angle between player view and enemy 
        float enemyAngle = atan2(dy, dx) - playerAngle;

        while (enemyAngle > PI)  enemyAngle -= 2 * PI;
        while (enemyAngle < -PI) enemyAngle += 2 * PI;

        // Check if enemy is inside FOV 
        if (fabs(enemyAngle) > halfFov){
            //std::cout<<"enemy out of FOV\n";
            continue; 
        }
        // Project enemy into screen space
        int screenX = (int)(
            (enemyAngle + halfFov) / fovRad * ScreenHeightWidth.first
        );

        // Perspective scaling 
        int spriteHeight = (int)(ScreenHeightWidth.second / enemyDist);
        int spriteWidth  = spriteHeight;

        int drawStartY = -spriteHeight / 2 + ScreenHeightWidth.second / 2;
        int drawEndY   =  spriteHeight / 2 + ScreenHeightWidth.second / 2;

        if (drawStartY < 0) drawStartY = 0;
        if (drawEndY >= ScreenHeightWidth.second)
            drawEndY = ScreenHeightWidth.second - 1;

        int drawStartX = -spriteWidth / 2 + screenX;
        int drawEndX   =  spriteWidth / 2 + screenX;

        // Select enemy texture 
        if (enemyTextures.empty()){
            //std::cout<<"0 elements in enemyTextures\n";
            continue;
        }
        int frame = enemy->get_current_frame(), dir = enemy->get_dirn_num();
        auto it = enemyTextures.find({frame, dir});
        if (it == enemyTextures.end()) continue;
        SDL_Texture* tex = it->second.get();

        // distance-based shading 
        float maxLightDist = 8.0f;
        float shade = 1.0f - std::min(enemyDist / maxLightDist, 1.0f);
        Uint8 brightness = (Uint8)(40 + shade * 215);
        SDL_SetTextureColorMod(tex,
                            brightness, brightness, brightness);
        
        int texW, texH;
        SDL_QueryTexture(tex, nullptr, nullptr, &texW, &texH);

        // Draw sprite column-by-column
        for (int x = drawStartX; x < drawEndX; x++)
        {
            if (x < 0 || x >= ScreenHeightWidth.first)
                continue;

            // Z-buffer check (VERY IMPORTANT) 
            if (enemyDist >= zBuffer[x])
                continue; 

            int texX = (int)(
                (x - drawStartX) * texW / spriteWidth
            );

            SDL_Rect srcRect = { texX, 0, 1, texH };
            SDL_Rect dstRect = {
                x,
                drawStartY,
                1,
                drawEndY - drawStartY
            };

            SDL_RenderCopy(renderer.get(), tex, &srcRect, &dstRect);
        }

    }

    SDL_RenderPresent(renderer.get()); 
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
void Game::addWallTexture(const char* filePath)
{
    SDL_Texture* raw = IMG_LoadTexture(renderer.get(), filePath);
    if (!raw) {
        std::cerr << "Failed to load wall texture: "
                  << filePath << " | " << IMG_GetError() << "\n";
        return;
    }

    wallTextures.emplace_back(raw, SDL_DestroyTexture);

    int width = 0, height = 0;
    if (SDL_QueryTexture(raw, nullptr, nullptr, &width, &height) != 0) {
        std::cerr << "Failed to query texture: "
                  << SDL_GetError() << "\n";
        wallTextures.pop_back();   // keep vectors in sync
        return;
    }

    wallTextureWidths.push_back(width);
    wallTextureHeights.push_back(height);
}

void Game::addFloorTexture(const char* filePath) {
    SDL_Texture* raw = IMG_LoadTexture(renderer.get(), filePath);
    if (!raw) {
        std::cerr << "Failed to load floor texture: "
                  << filePath << " | " << IMG_GetError() << "\n";
        return;
    }

    floorTextures.emplace_back(raw, SDL_DestroyTexture);

    int width = 0, height = 0;
    if (SDL_QueryTexture(raw, nullptr, nullptr, &width, &height) != 0) {
        std::cerr << "Failed to query texture: "
                  << SDL_GetError() << "\n";
        floorTextures.pop_back();   // keep vectors in sync
        return;
    }

    floorTextureWidths.push_back(width);
    floorTextureHeights.push_back(height);
}
void Game::addCeilingTexture(const char* filePath) {
    SDL_Texture* raw = IMG_LoadTexture(renderer.get(), filePath);
    if (!raw) {
        std::cerr << "Failed to load ceiling texture: "
                  << filePath << " | " << IMG_GetError() << "\n";
        return;
    }

    ceilingTextures.emplace_back(raw, SDL_DestroyTexture);

    int width = 0, height = 0;
    if (SDL_QueryTexture(raw, nullptr, nullptr, &width, &height) != 0) {
        std::cerr << "Failed to query texture: "
                  << SDL_GetError() << "\n";
        ceilingTextures.pop_back();   // keep vectors in sync
        return;
    }

    ceilingTextureWidths.push_back(width);
    ceilingTextureHeights.push_back(height);
}

void Game::printPlayerPosition(){
    std::cout << "Player Position: (" << playerPosition.first << ", " << playerPosition.second << ")\n";
}
bool Game::isDoor(int tile) {
    return tile >= 6 && tile <=9;
}
bool Game::playerHasKey(int keyType) {
    if(keyType == 0) return true; // no key needed
    for (int key : keysHeld) {
        if (key == keyType) {
            return true;
        }
    }
    return false;
}
static std::string toLower(const std::string &s) {
    std::string r = s;
    std::transform(r.begin(), r.end(), r.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return r;
}
void Game::loadAllTextures(const char* filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open texture list file: " << filePath << "\n";
        return;
    }

    enum Section { NONE, WALLS, FLOORS, CEILS };
    Section currentSection = NONE;

    std::string line;
    while (std::getline(file, line)) {

        // Trim leading/trailing spaces
        auto trim = [](std::string &s) {
            while (!s.empty() && std::isspace((unsigned char)s.front())) s.erase(s.begin());
            while (!s.empty() && std::isspace((unsigned char)s.back())) s.pop_back();
        };
        trim(line);
        if (line.empty()) continue;       // Skip blank lines

        std::string low = toLower(line);

        // Detect section headers case-insensitively
        if (low == "[walls]") {
            currentSection = WALLS;
            continue;
        }
        if (low == "[floors]") {
            currentSection = FLOORS;
            continue;
        }
        if (low == "[ceils]" || low == "[ceil]" || low == "[ceilings]") {
            currentSection = CEILS;
            continue;
        }

        // If it’s not a section header, it must be a file path
        if (currentSection == WALLS) {
            addWallTexture(line.c_str());
        }
        else if (currentSection == FLOORS) {
            addFloorTexture(line.c_str());
        }
        else if (currentSection == CEILS) {
            addCeilingTexture(line.c_str());
        }
        else {
            std::cerr << "Warning: Path found outside any valid section: " << line << "\n";
        }
    }
}
void Game::clean()
{
    enemyTextures.clear();
    wallTextures.clear();
    floorTextures.clear();
    ceilingTextures.clear();
    doors.clear();
    enemies.clear();

    renderer.reset();
    window.reset();

    IMG_Quit();
    SDL_Quit();

}

void Game::loadEnemyTextures(const char* filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filePath << "\n";
        return;
    }

    std::string line;

    while (std::getline(file, line)) {

        // Remove UTF-8 BOM if present (important for first line)
        if (!line.empty() && static_cast<unsigned char>(line[0]) == 0xEF)
            line.erase(0, 3);

        // Remove comments
        auto comment_pos = line.find('#');
        if (comment_pos != std::string::npos)
            line = line.substr(0, comment_pos);

        std::istringstream iss(line);

        int a, b;
        std::string path;

        // Expect: <int> <int> <string>
        if (iss >> a >> b >> path) {
            SDL_Texture* texture = IMG_LoadTexture(renderer.get(), path.c_str());
            if (!texture) {
                std::cerr << "Failed to load texture: " << filePath << " Error: " << IMG_GetError() << std::endl;
                return;
            }
            enemyTextures.insert_or_assign(
                {a, b},
                SDLTexturePtr(texture, SDL_DestroyTexture)
            );
        }
        // else: silently ignore malformed / empty lines
    }
    //std::cout<<enemyTextures.size()<<std::endl;
}

void Game::addEnemy(float x, float y, float angle) {
    enemies.push_back(std::make_unique<Enemy>(x, y, angle));
}

bool aabbIntersect(
    float ax, float ay, float aw, float ah,
    float bx, float by, float bw, float bh
) {
    return ax < bx + bw &&
           ax + aw > bx &&
           ay < by + bh &&
           ay + ah > by;
}

bool Game::collidesWithEnemy(float x, float y) {
    for (const auto& e : enemies)
    {
        if (aabbIntersect(
            x, y,
            playerSquareSize, playerSquareSize,
            e->get_position().first, e->get_position().second,
            e->get_size(), e->get_size()
        )) {
            return true;
        }
    }
    return false;
}

bool Game::rayCastEnemyToPlayer(
    const Enemy& enemy,
    const std::pair<float,float>& playerPos
) {
    float ex = enemy.get_position().first;
    float ey = enemy.get_position().second;

    float px = playerPos.first;
    float py = playerPos.second;

    // Direction vector
    float dx = px - ex;
    float dy = py - ey;

    float rayLength = std::hypot(dx, dy);
    if (rayLength < 0.0001f)
        return true;

    dx /= rayLength;
    dy /= rayLength;

    // Current grid square
    int mapX = int(std::floor(ex));
    int mapY = int(std::floor(ey));

    int targetX = int(std::floor(px));
    int targetY = int(std::floor(py));

    // Ray step direction
    int stepX = (dx < 0) ? -1 : 1;
    int stepY = (dy < 0) ? -1 : 1;

    // Distance to first grid boundary
    float sideDistX;
    float sideDistY;

    float deltaDistX = (dx == 0) ? 1e30f : std::abs(1.0f / dx);
    float deltaDistY = (dy == 0) ? 1e30f : std::abs(1.0f / dy);

    if (dx < 0)
        sideDistX = (ex - mapX) * deltaDistX;
    else
        sideDistX = (mapX + 1.0f - ex) * deltaDistX;

    if (dy < 0)
        sideDistY = (ey - mapY) * deltaDistY;
    else
        sideDistY = (mapY + 1.0f - ey) * deltaDistY;

    // DDA loop
    while (true) {
        if (sideDistX < sideDistY) {
            sideDistX += deltaDistX;
            mapX += stepX;
        } else {
            sideDistY += deltaDistY;
            mapY += stepY;
        }

        // Bounds check
        if (mapY < 0 || mapY >= (int)Map.size() ||
            mapX < 0 || mapX >= (int)Map[0].size())
            return false;

        // Hit wall
        if (Map[mapY][mapX] != 0)
            return false;

        // Reached player cell
        if (mapX == targetX && mapY == targetY)
            return true;
    }
}

/*
TODO:
UPDATE ALERTED
APPLY DAMAGES TO PLAYER AND ENEMIES
*/