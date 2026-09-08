#include "game_scene.h"

using namespace qc;

#include "resources.h"

#include <algorithm>
#include <limits>

namespace game {
namespace {

Model BuildEditableModel(const qscene::MeshComponent& source) {
    Mesh mesh{};
    mesh.vertexCount = static_cast<int>(source.editable_vertices.size());
    mesh.triangleCount = static_cast<int>(source.editable_indices.size() / 3);
    if (mesh.vertexCount == 0 || mesh.triangleCount == 0) return Model{};

    mesh.vertices = new float[mesh.vertexCount * 3]{};
    mesh.texcoords = new float[mesh.vertexCount * 2]{};
    mesh.normals = new float[mesh.vertexCount * 3]{};
    mesh.indices = new unsigned short[mesh.triangleCount * 3]{};
    for (int vertexIndex = 0; vertexIndex < mesh.vertexCount; ++vertexIndex) {
        const Vec3& position = source.editable_vertices[vertexIndex];
        const Vec2& texcoord = source.editable_texcoords[vertexIndex];
        mesh.vertices[vertexIndex * 3] = position.x;
        mesh.vertices[vertexIndex * 3 + 1] = position.y;
        mesh.vertices[vertexIndex * 3 + 2] = position.z;
        mesh.texcoords[vertexIndex * 2] = texcoord.x;
        mesh.texcoords[vertexIndex * 2 + 1] = texcoord.y;
    }
    for (int index = 0; index < mesh.triangleCount * 3; ++index)
        mesh.indices[index] = source.editable_indices[index];

    for (int triangle = 0; triangle < mesh.triangleCount; ++triangle) {
        const unsigned short first = mesh.indices[triangle * 3];
        const unsigned short second = mesh.indices[triangle * 3 + 1];
        const unsigned short third = mesh.indices[triangle * 3 + 2];
        if (first >= mesh.vertexCount || second >= mesh.vertexCount || third >= mesh.vertexCount) continue;
        const Vec3 edgeA{mesh.vertices[second * 3] - mesh.vertices[first * 3],
            mesh.vertices[second * 3 + 1] - mesh.vertices[first * 3 + 1],
            mesh.vertices[second * 3 + 2] - mesh.vertices[first * 3 + 2]};
        const Vec3 edgeB{mesh.vertices[third * 3] - mesh.vertices[first * 3],
            mesh.vertices[third * 3 + 1] - mesh.vertices[first * 3 + 1],
            mesh.vertices[third * 3 + 2] - mesh.vertices[first * 3 + 2]};
        const Vec3 normal = edgeA.cross(edgeB).normalized();
        for (unsigned short vertex : {first, second, third}) {
            mesh.normals[vertex * 3] += normal.x;
            mesh.normals[vertex * 3 + 1] += normal.y;
            mesh.normals[vertex * 3 + 2] += normal.z;
        }
    }
    for (int vertexIndex = 0; vertexIndex < mesh.vertexCount; ++vertexIndex) {
        Vec3 normal{mesh.normals[vertexIndex * 3], mesh.normals[vertexIndex * 3 + 1], mesh.normals[vertexIndex * 3 + 2]};
        normal = normal.normalized();
        mesh.normals[vertexIndex * 3] = normal.x;
        mesh.normals[vertexIndex * 3 + 1] = normal.y;
        mesh.normals[vertexIndex * 3 + 2] = normal.z;
    }
    return LoadModelFromMesh(mesh);
}

}

bool GameScene::Initialize() {
    gs_Resources.Load<qscene::Scene>("scene", "resources/scenes/scene.json");
    m_pScene = &gs_Resources.Get<qscene::Scene>("scene");

    m_LightingShader = LoadShader("resources/shaders/lighting.vs", "resources/shaders/lighting.fs");
    m_ShadowShader = LoadShader("resources/shaders/shadow_depth.vs", "resources/shaders/shadow_depth.fs");
    if (!IsShaderValid(m_LightingShader) || !IsShaderValid(m_ShadowShader)) {
        TraceLog(LogLevel::Error, "SCENE", "Could not load lighting shaders");
        return false;
    }

    Mesh cubeMesh = GenMeshCube(1.0f, 1.0f, 1.0f);
    m_PrimitiveModel = LoadModelFromMesh(cubeMesh);
    for (int entityIndex = 0; entityIndex < static_cast<int>(m_pScene->entities.size()); ++entityIndex) {
        const auto& entity = m_pScene->entities[entityIndex];
        if (entity.mesh && entity.mesh->enabled && entity.mesh->is_primitive && entity.mesh->is_editable_mesh)
            m_vEditablePrimitiveModels.emplace(entityIndex, BuildEditableModel(*entity.mesh));
    }

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

    std::vector<Player::Collider> colliders;
    for (int entityIndex = 0; entityIndex < static_cast<int>(m_pScene->entities.size()); ++entityIndex) {
        const auto& entity = m_pScene->entities[entityIndex];
        if (!entity.mesh || !entity.mesh->enabled) continue;

        const Matrix worldTransform = qscene::SceneLoader::BuildWorldTransformMatrix(*m_pScene, entityIndex);
        Vec3 boundsMin{std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
            std::numeric_limits<float>::max()};
        Vec3 boundsMax{std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(),
            std::numeric_limits<float>::lowest()};
        const auto includePoint = [&](const Vec3& point) {
            const Vec3 worldPoint = Vec3Transform(point, worldTransform);
            boundsMin.x = std::min(boundsMin.x, worldPoint.x);
            boundsMin.y = std::min(boundsMin.y, worldPoint.y);
            boundsMin.z = std::min(boundsMin.z, worldPoint.z);
            boundsMax.x = std::max(boundsMax.x, worldPoint.x);
            boundsMax.y = std::max(boundsMax.y, worldPoint.y);
            boundsMax.z = std::max(boundsMax.z, worldPoint.z);
        };
        std::vector<Player::Collider::Triangle> triangles;
        const auto includeTriangle = [&](const Vec3& a, const Vec3& b, const Vec3& c) {
            triangles.push_back(Player::Collider::Triangle{
                Vec3Transform(a, worldTransform), Vec3Transform(b, worldTransform),
                Vec3Transform(c, worldTransform)});
        };

        if (entity.mesh->is_primitive) {
            if (!entity.mesh->editable_vertices.empty()) {
                for (const Vec3& vertex : entity.mesh->editable_vertices) includePoint(vertex);
                for (size_t index = 0; index + 2 < entity.mesh->editable_indices.size(); index += 3) {
                    const unsigned short first = entity.mesh->editable_indices[index];
                    const unsigned short second = entity.mesh->editable_indices[index + 1];
                    const unsigned short third = entity.mesh->editable_indices[index + 2];
                    if (first < entity.mesh->editable_vertices.size() &&
                        second < entity.mesh->editable_vertices.size() &&
                        third < entity.mesh->editable_vertices.size())
                        includeTriangle(entity.mesh->editable_vertices[first],
                            entity.mesh->editable_vertices[second], entity.mesh->editable_vertices[third]);
                }
            } else {
                const Mesh& mesh = m_PrimitiveModel.meshes[0];
                for (int index = 0; index < mesh.triangleCount * 3; index += 3)
                    includeTriangle(
                        Vec3{mesh.vertices[mesh.indices[index] * 3], mesh.vertices[mesh.indices[index] * 3 + 1], mesh.vertices[mesh.indices[index] * 3 + 2]},
                        Vec3{mesh.vertices[mesh.indices[index + 1] * 3], mesh.vertices[mesh.indices[index + 1] * 3 + 1], mesh.vertices[mesh.indices[index + 1] * 3 + 2]},
                        Vec3{mesh.vertices[mesh.indices[index + 2] * 3], mesh.vertices[mesh.indices[index + 2] * 3 + 1], mesh.vertices[mesh.indices[index + 2] * 3 + 2]});
                for (int x : {-1, 1})
                    for (int y : {-1, 1})
                        for (int z : {-1, 1})
                            includePoint(Vec3{0.5f * x, 0.5f * y, 0.5f * z});
            }
        } else {
            const std::string path = qscene::SceneLoader::RemapAssetPath(entity.mesh->asset_name);
            if (!gs_Resources.Has<Model>(path)) continue;
            const BoundingBox modelBounds = GetModelBoundingBox(gs_Resources.Get<Model>(path));
            for (int meshIndex = 0; meshIndex < gs_Resources.Get<Model>(path).meshCount; ++meshIndex) {
                const Mesh& mesh = gs_Resources.Get<Model>(path).meshes[meshIndex];
                for (int index = 0; index < mesh.triangleCount * 3; index += 3) {
                    const auto vertex = [&](int vertexIndex) {
                        return Vec3{mesh.vertices[vertexIndex * 3], mesh.vertices[vertexIndex * 3 + 1],
                            mesh.vertices[vertexIndex * 3 + 2]};
                    };
                    const int first = mesh.indices ? mesh.indices[index] : index;
                    const int second = mesh.indices ? mesh.indices[index + 1] : index + 1;
                    const int third = mesh.indices ? mesh.indices[index + 2] : index + 2;
                    includeTriangle(vertex(first), vertex(second), vertex(third));
                }
            }
            for (int x : {0, 1})
                for (int y : {0, 1})
                    for (int z : {0, 1})
                        includePoint(Vec3{
                            x == 0 ? modelBounds.min.x : modelBounds.max.x,
                            y == 0 ? modelBounds.min.y : modelBounds.max.y,
                            z == 0 ? modelBounds.min.z : modelBounds.max.z});
        }

        colliders.push_back(Player::Collider{boundsMin, boundsMax,
            std::find(entity.tags.begin(), entity.tags.end(), "Ground") != entity.tags.end(),
            std::move(triangles)});
    }
    m_Player.SetColliders(colliders);

    m_Lightning.Initialize(*m_pScene);
    m_Lightning.PrepareModel(m_PrimitiveModel, m_LightingShader);
    for (auto& [entityIndex, model] : m_vEditablePrimitiveModels)
        m_Lightning.PrepareModel(model, m_LightingShader);
    for (const auto& path : m_vModelPaths)
        m_Lightning.PrepareModel(gs_Resources.Get<Model>(path), m_LightingShader);

    return true;
}

void GameScene::Update() {
    m_Player.Update();
}

void GameScene::Draw() {
    m_Lightning.RenderShadowPass(*m_pScene, m_PrimitiveModel,
        m_vEditablePrimitiveModels, m_ShadowShader);
    m_Lightning.PrepareModel(m_PrimitiveModel, m_LightingShader);
    for (auto& [entityIndex, model] : m_vEditablePrimitiveModels)
        m_Lightning.PrepareModel(model, m_LightingShader);
    for (const auto& path : m_vModelPaths)
        m_Lightning.PrepareModel(gs_Resources.Get<Model>(path), m_LightingShader);

    BeginDrawing();
    ClearBackground(BLACK);
    BeginMode3D(m_Player.GetCamera());

    Camera3D camera = m_Player.GetCamera();
    m_Lightning.ApplyLighting(m_LightingShader, camera);

    for (int entityIndex = 0; entityIndex < static_cast<int>(m_pScene->entities.size()); ++entityIndex) {
        const auto& entity = m_pScene->entities[entityIndex];
        if (!entity.mesh || !entity.mesh->enabled) continue;

        Matrix transform = qscene::SceneLoader::BuildWorldTransformMatrix(*m_pScene, entityIndex);
        Color tint = WHITE;
        if (entity.material && entity.material->enabled) {
            tint = Color{entity.material->color.r, entity.material->color.g,
                entity.material->color.b, entity.material->color.a};
        }

        if (entity.mesh->is_primitive) {
                Model* primitiveModel = &m_PrimitiveModel;
                auto editableModel = m_vEditablePrimitiveModels.find(entityIndex);
                if (editableModel != m_vEditablePrimitiveModels.end()) primitiveModel = &editableModel->second;
            if (entity.material && entity.material->enabled)
                    qscene::SceneLoader::ApplySceneMaterial(*primitiveModel, *entity.material);
                primitiveModel->transform = transform;
                DrawModel(*primitiveModel, Vec3{0, 0, 0}, 1.0f, tint);
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
    for (auto& [entityIndex, model] : m_vEditablePrimitiveModels)
        if (model.meshCount != 0) UnloadModel(model);
    m_vEditablePrimitiveModels.clear();
    m_Lightning.Shutdown();
    if (IsShaderValid(m_LightingShader)) UnloadShader(m_LightingShader);
    if (IsShaderValid(m_ShadowShader)) UnloadShader(m_ShadowShader);
    gs_Resources.UnloadAll();
}

} // namespace game