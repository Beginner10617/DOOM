#include "WolfGame.hpp"

Game* game = nullptr;

int main(int argc, char* argv[]) {
    game = new Game();
    game->addEnemy(5.0f,5.0f,0.0f);
    game->init("My Game", 100, 100, 800, 600, false);
    game->placePlayerAt(2, 2, 0.0f);
    game->loadMapDataFromFile("testMap.txt");
    game->loadAllTextures("textureMapping.txt");
    game->loadEnemyTextures("enemy.txt");

    const int FPS = 60;
    const float frameDelay = 1000.0f / FPS;

    Uint32 lastTicks = SDL_GetTicks();

    while (game->running()) {
        Uint32 frameStart = SDL_GetTicks();

        // Delta Time calculation
        Uint32 currentTicks = frameStart;
        float deltaTime = (currentTicks - lastTicks) / 1000.0f;  
        lastTicks = currentTicks;

        // Game Loop 
        game->handleEvents();
        game->update(deltaTime);   
        game->render();

        // Frame Limiter 
        Uint32 frameTime = SDL_GetTicks() - frameStart;
        
        if (frameTime < frameDelay) {
            SDL_Delay(frameDelay - frameTime);
        }
    }

    game->clean();
    delete game;
    return 0;
}
