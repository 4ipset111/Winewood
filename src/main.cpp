#include "QuarkCore/QuarkCore.hpp"
#include "resources.h"

using namespace qc;

int main()
{
	InitWindow(1280, 720, "Winewood", RendererType::OpenGL);
    SetTargetFPS(GetMonitorRefreshRate(GetCurrentMonitor()));

	Resources.Load<Texture2D>("test", "resources/test.png");

	while (!WindowShouldClose())
	{
		BeginDrawing();
		ClearBackground(Color{20, 24, 32, 255});

        DrawDebugText(TextFormat("%d", GetFPS()), 0, 0, 24, Color{255, 255, 255, 255});
        DrawDebugText("Winewood", 0, 24, 24, Color{255, 255, 255, 255});

		auto& tex = Resources.Get<Texture2D>("test");
		DrawTexture(tex, 0, 0, WHITE);
		
		EndDrawing();
		}

	CloseWindow();
	return 0;
}
