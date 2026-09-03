#include "game_scene.h"

using namespace qc;

#include "resources.h"

#include <cmath>

namespace game {

bool GameScene::Initialize() {
    gs_Resources.Load<qscene::Scene>("scene", "resources/scenes/scene.json");
    m_pScene = &gs_Resources.Get<qscene::Scene>("scene");

    m_LightingShader = LoadShader("resources/shaders/lighting.vs", "resources/shaders/lighting.fs");
    m_ShadowShader = LoadShader("resources/shaders/shadow_depth.vs", "resources/shaders/shadow_depth.fs");
    if (!IsShaderValid(m_LightingShader) || !IsShaderValid(m_ShadowShader)) {
        TraceLog(LogLevel::Error, "SCENE", "Could not load lighting shaders");
        return false;
    }

    std::vector<Player::Collider> colliders;
    for (const auto& entity : m_pScene->entities) {
        if (!entity.mesh || !entity.mesh->enabled || !entity.mesh->is_primitive) continue;

        Vec3 halfSize{
            std::abs(entity.transform.scale.x) * 0.5f,
            std::abs(entity.transform.scale.y) * 0.5f,
            std::abs(entity.transform.scale.z) * 0.5f};
        colliders.push_back(Player::Collider{
            entity.transform.position - halfSize,
            entity.transform.position + halfSize});
    }
    m_Player.SetColliders(colliders);

    Mesh cubeMesh = GenMeshCube(1.0f, 1.0f, 1.0f);
    m_PrimitiveModel = LoadModelFromMesh(cubeMesh);

    for (const auto& entity : m_pScene->entities) {
        if (!entity.mesh || !entity.mesh->enabled || entity.mesh->is_primitive) continue;

        std::string path = qscene::SceneLoader::RemapAssetPath(entity.mesh->asset_name);
        if (gs_Resources.Has<Model>(path)) continue;

        bool loaded = false;
        if (FileExists(path.c_str())) {
            gs_Resources.Load<Model>(path, path);
            loaded = true;
        } else if (path != entity.mesh->asset_name && FileExists(entity.mesh->asset_name.c_str())) {
            gs_Resources.Load<Model>(path, entity.mesh->asset_name);
            loaded = true;
        }

        if (loaded) {
            m_vModelPaths.push_back(path);
        } else {
            TraceLog(LogLevel::Warn, "SCENE",
                TextFormat("Scene asset not found: %s (original: %s)",
                    path.c_str(), entity.mesh->asset_name.c_str()));
        }
    }

    m_Lightning.Initialize(*m_pScene);
    m_Lightning.PrepareModel(m_PrimitiveModel, m_LightingShader);
    for (const auto& path : m_vModelPaths)
        m_Lightning.PrepareModel(gs_Resources.Get<Model>(path), m_LightingShader);
    return true;
}

void GameScene::Update() {
    m_Player.Update();
}

void GameScene::Draw() {
    m_Lightning.RenderShadowPass(*m_pScene, m_PrimitiveModel, m_ShadowShader);
    m_Lightning.PrepareModel(m_PrimitiveModel, m_LightingShader);
    for (const auto& path : m_vModelPaths)
        m_Lightning.PrepareModel(gs_Resources.Get<Model>(path), m_LightingShader);

    BeginDrawing();
    ClearBackground(BLACK);
    BeginMode3D(m_Player.GetCamera());

    Camera3D camera = m_Player.GetCamera();
    m_Lightning.ApplyLighting(m_LightingShader, camera);

    for (const auto& entity : m_pScene->entities) {
        if (!entity.mesh || !entity.mesh->enabled) continue;

        Matrix transform = qscene::SceneLoader::BuildTransformMatrix(entity.transform);
        Color tint = WHITE;
        if (entity.material && entity.material->enabled) {
            tint = Color{entity.material->color.r, entity.material->color.g,
                entity.material->color.b, entity.material->color.a};
        }

        if (entity.mesh->is_primitive) {
            if (entity.material && entity.material->enabled)
                qscene::SceneLoader::ApplySceneMaterial(m_PrimitiveModel, *entity.material);
            m_PrimitiveModel.transform = transform;
            DrawModel(m_PrimitiveModel, Vec3{0, 0, 0}, 1.0f, tint);
        } else {
            std::string path = qscene::SceneLoader::RemapAssetPath(entity.mesh->asset_name);
            if (!gs_Resources.Has<Model>(path)) continue;

            Model& model = gs_Resources.Get<Model>(path);
            if (entity.material && entity.material->enabled)
                qscene::SceneLoader::ApplySceneMaterial(model, *entity.material);
            model.transform = transform;
            DrawModel(model, Vec3{0, 0, 0}, 1.0f, tint);
        }
    }

    m_Lightning.DrawDebugLights(*m_pScene);

    m_Player.DrawDebugCollision(RED);
    EndMode3D();

    DrawDebugText(TextFormat("%d", GetFPS()), 0, 0, 24, Color{255, 255, 255, 255});
    DrawDebugText(TextFormat("Position: (%.2f, %.2f, %.2f)", m_Player.GetPosition().x,
        m_Player.GetPosition().y, m_Player.GetPosition().z), 0, 30, 24, Color{255, 255, 255, 255});
    EndDrawing();
}

void GameScene::Shutdown() {
    if (m_PrimitiveModel.meshCount != 0) UnloadModel(m_PrimitiveModel);
    m_Lightning.Shutdown();
    if (IsShaderValid(m_LightingShader)) UnloadShader(m_LightingShader);
    if (IsShaderValid(m_ShadowShader)) UnloadShader(m_ShadowShader);
    gs_Resources.UnloadAll();
}

} // namespace game
