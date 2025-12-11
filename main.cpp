#include "WolfGame.hpp"

Game* game = nullptr;

int main(int argc, char* argv[]) {
    game = new Game();
    game->init("My Game", 100, 100, 800, 600, false);
    game->placePlayerAt(5, 5, 0.0f);
    game->loadMapDataFromFile("testMap.txt");
    for(int i=0; i<5; i++)
        game->addWallTexture("Textures/zzwolf11.png");
    game->addWallTexture("Textures/vp0yqlw9SToP7ejS.jpg");
    game->addFloorTexture("Textures/floor0_5.png");
    game->addCeilingTexture("Textures/compblue.png");
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
