#include "QuarkCore/QuarkCore.hpp"
#include "SDL3/SDL.h"
#include "resources.h"
#include "player.h"

using namespace qc;

int main()
{
	InitWindow(1280, 720, "Winewood", RendererType::OpenGL);
	SDL_SetWindowRelativeMouseMode(GetNativeWindow(), true);
	SetTargetFPS(GetMonitorRefreshRate(GetCurrentMonitor()));
	DisableCursor();

	Player player(Vec3{ 0.0f, 0.0f, 0.0f });

	while (!WindowShouldClose())
	{
		player.Update();

		BeginDrawing();
		ClearBackground(Color{ 20, 24, 32, 255 });

		BeginMode3D(player.GetCamera());
		DrawCube(Vec3{0, 0, 0}, 2, 2, 2, RED);
		EndMode3D();

		DrawDebugText(TextFormat("%d", GetFPS()), 0, 0, 24, Color{ 255, 255, 255, 255 });
		DrawDebugText("Winewood", 0, 24, 24, Color{ 255, 255, 255, 255 });

		EndDrawing();
	}

	CloseWindow();
	return 0;
}