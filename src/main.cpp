#include "SDL3/SDL.h"
#include "game_scene.h"
using namespace qc;

int main() {
    InitWindow(1280, 720, "Winewood", RendererType::OpenGL);
    SetTargetFPS(GetMonitorRefreshRate(GetCurrentMonitor()));
    SDL_SetWindowRelativeMouseMode(GetNativeWindow(), true);
    DisableCursor();

    game::GameScene scene;
    if (!scene.Initialize()) {
        CloseWindow();
        return 1;
    }

    while (!WindowShouldClose()) {
        scene.Update();
        scene.Draw();
    }

    scene.Shutdown();
    CloseWindow();
    return 0;
}