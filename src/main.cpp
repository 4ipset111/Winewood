#include "QuarkCore/QuarkCore.hpp"
#include "resources.h"

using namespace qc;

int main()
{
	InitWindow(1280, 720, "Winewood", RendererType::OpenGL);
    SetTargetFPS(GetMonitorRefreshRate(GetCurrentMonitor()));

	while (!WindowShouldClose())
	{
		BeginDrawing();
		ClearBackground(Color{20, 24, 32, 255});

        DrawDebugText(TextFormat("%d", GetFPS()), 0, 0, 24, Color{255, 255, 255, 255});
        DrawDebugText("Winewood", 0, 24, 24, Color{255, 255, 255, 255});

		EndDrawing();
	}

	CloseWindow();
	return 0;
}
