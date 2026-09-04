#include "game_scene.h"

using namespace qc;

#include "resources.h"

#include <array>
#include <algorithm>
#include <cmath>
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

    m_HandModel = LoadModel("resources/models/hand/hand.obj");
    if (!IsModelValid(m_HandModel)) {
        TraceLog(LogLevel::Error, "SCENE", "Could not load first-person hand model");
        return false;
    }
    Texture2D handTexture = qscene::SceneLoader::LoadMaterialTexture(
        "resources/models/hand/hand.png");
    if (handTexture.valid && m_HandModel.materialCount > 0 && m_HandModel.materials[0].maps) {
        m_HandModel.materials[0].maps[MATERIAL_MAP_ALBEDO].color = WHITE;
        m_HandModel.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = handTexture;
    }
    gs_Resources.Load<Texture2D>("bullet_impact", "resources/textures/Bullet_Impact.png");
    m_BulletImpactTexture = gs_Resources.Get<Texture2D>("bullet_impact");
    m_ImpactModel = LoadModelFromMesh(GenMeshPlane(1.0f, 1.0f, 1, 1));
    if (m_ImpactModel.materialCount > 0 && m_ImpactModel.materials[0].maps) {
        m_ImpactModel.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = m_BulletImpactTexture;
        m_ImpactModel.materials[0].maps[MATERIAL_MAP_ALBEDO].color = WHITE;
    }

    m_pWeapon = new TokarevWeapon();
    if (!m_pWeapon->Initialize())
        return false;

    std::vector<Player::Collider> colliders;
    for (int entityIndex = 0; entityIndex < static_cast<int>(m_pScene->entities.size()); ++entityIndex) {
        const auto& entity = m_pScene->entities[entityIndex];
        if (!entity.mesh || !entity.mesh->enabled || !entity.mesh->is_primitive) continue;

        const Matrix worldTransform = qscene::SceneLoader::BuildWorldTransformMatrix(*m_pScene, entityIndex);
        Vec3 halfSize{
            std::sqrt(worldTransform.m[0] * worldTransform.m[0] + worldTransform.m[4] * worldTransform.m[4] + worldTransform.m[8] * worldTransform.m[8]) * 0.5f,
            std::sqrt(worldTransform.m[1] * worldTransform.m[1] + worldTransform.m[5] * worldTransform.m[5] + worldTransform.m[9] * worldTransform.m[9]) * 0.5f,
            std::sqrt(worldTransform.m[2] * worldTransform.m[2] + worldTransform.m[6] * worldTransform.m[6] + worldTransform.m[10] * worldTransform.m[10]) * 0.5f};
        const Vec3 worldPosition{worldTransform.m[12], worldTransform.m[13], worldTransform.m[14]};
        colliders.push_back(Player::Collider{
            worldPosition - halfSize,
            worldPosition + halfSize});
    }
    m_Player.SetColliders(colliders);

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

    m_Lightning.Initialize(*m_pScene);
    m_Lightning.PrepareModel(m_PrimitiveModel, m_LightingShader);
    for (auto& [entityIndex, model] : m_vEditablePrimitiveModels)
        m_Lightning.PrepareModel(model, m_LightingShader);
    m_Lightning.PrepareModel(m_HandModel, m_LightingShader);
    m_Lightning.PrepareModel(m_ImpactModel, m_LightingShader);
    for (const auto& path : m_vModelPaths)
        m_Lightning.PrepareModel(gs_Resources.Get<Model>(path), m_LightingShader);
    return true;
}

void GameScene::Update() {
    m_Player.Update();

    for (auto& impact : m_vImpactMarks)
        impact.lifetime -= GetFrameTime();
    m_vImpactMarks.erase(std::remove_if(m_vImpactMarks.begin(), m_vImpactMarks.end(),
        [](const ImpactMark& impact) { return impact.lifetime <= 0.0f; }), m_vImpactMarks.end());

    const std::optional<WeaponShot> shot =
        m_pWeapon->Update(m_Player.GetCamera(), GetFrameTime());
    if (shot) {
        m_Player.ApplyRecoil(shot->recoilPitch);

        const Ray& ray = shot->ray;
        RayCollision nearest{};
        nearest.distance = std::numeric_limits<float>::max();
        auto testModel = [&](const Model& model, const Matrix& transform) {
            for (int meshIndex = 0; meshIndex < model.meshCount; ++meshIndex) {
                RayCollision collision = GetRayCollisionMesh(ray, model.meshes[meshIndex], transform);
                if (collision.hit && collision.distance < nearest.distance)
                    nearest = collision;
            }
        };

        for (int entityIndex = 0; entityIndex < static_cast<int>(m_pScene->entities.size()); ++entityIndex) {
            const auto& entity = m_pScene->entities[entityIndex];
            if (!entity.mesh || !entity.mesh->enabled) continue;
            Matrix transform = qscene::SceneLoader::BuildWorldTransformMatrix(*m_pScene, entityIndex);
            if (entity.mesh->is_primitive) {
                const Model* modelToTest = &m_PrimitiveModel;
                auto editableModel = m_vEditablePrimitiveModels.find(entityIndex);
                if (editableModel != m_vEditablePrimitiveModels.end()) modelToTest = &editableModel->second;
                testModel(*modelToTest, transform);
            } else {
                const std::string path = qscene::SceneLoader::RemapAssetPath(entity.mesh->asset_name);
                if (gs_Resources.Has<Model>(path))
                    testModel(gs_Resources.Get<Model>(path), transform);
            }
        }

        if (nearest.hit) {
            m_vImpactMarks.push_back(ImpactMark{
                nearest.point + nearest.normal * 0.002f,
                nearest.normal,
                4.0f});
        }
    }
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

    for (const auto& impact : m_vImpactMarks) {
        const Vec3 worldUp{0.0f, 1.0f, 0.0f};
        const Vec3 fallbackAxis{1.0f, 0.0f, 0.0f};
        Vec3 tangent = impact.normal.cross(worldUp);
        if (tangent.length() < 0.01f)
            tangent = impact.normal.cross(fallbackAxis);
        tangent = tangent.normalized();
        const Vec3 bitangent = impact.normal.cross(tangent).normalized();
        constexpr float impactSize = 0.12f;

        m_ImpactModel.transform = Matrix::identity();
        m_ImpactModel.transform.m[0] = tangent.x * impactSize;
        m_ImpactModel.transform.m[1] = tangent.y * impactSize;
        m_ImpactModel.transform.m[2] = tangent.z * impactSize;
        m_ImpactModel.transform.m[4] = impact.normal.x * impactSize;
        m_ImpactModel.transform.m[5] = impact.normal.y * impactSize;
        m_ImpactModel.transform.m[6] = impact.normal.z * impactSize;
        m_ImpactModel.transform.m[8] = bitangent.x * impactSize;
        m_ImpactModel.transform.m[9] = bitangent.y * impactSize;
        m_ImpactModel.transform.m[10] = bitangent.z * impactSize;
        m_ImpactModel.transform.m[12] = impact.position.x;
        m_ImpactModel.transform.m[13] = impact.position.y;
        m_ImpactModel.transform.m[14] = impact.position.z;
        DrawModel(m_ImpactModel, Vec3{0, 0, 0}, 1.0f, WHITE);
    }

    const Vec3 cameraForward = (camera.target - camera.position).normalized();
    const Vec3 cameraRight = cameraForward.cross(camera.up).normalized();
    const Vec3 cameraUp = cameraRight.cross(cameraForward).normalized();
    const Vec3 handPosition = camera.position + cameraRight * 0.60f
        + cameraUp * -0.70f + cameraForward * 0.65f;
    constexpr float handScale = 0.14f;
    m_HandModel.transform = Matrix::identity();
    m_HandModel.transform.m[0] = -cameraRight.x * handScale;
    m_HandModel.transform.m[1] = -cameraRight.y * handScale;
    m_HandModel.transform.m[2] = -cameraRight.z * handScale;
    m_HandModel.transform.m[4] = cameraUp.x * handScale;
    m_HandModel.transform.m[5] = cameraUp.y * handScale;
    m_HandModel.transform.m[6] = cameraUp.z * handScale;
    m_HandModel.transform.m[8] = cameraForward.x * handScale;
    m_HandModel.transform.m[9] = cameraForward.y * handScale;
    m_HandModel.transform.m[10] = cameraForward.z * handScale;
    m_HandModel.transform.m[12] = handPosition.x;
    m_HandModel.transform.m[13] = handPosition.y;
    m_HandModel.transform.m[14] = handPosition.z;
    DrawModel(m_HandModel, Vec3{0, 0, 0}, 1.0f, WHITE);

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
    if (m_HandModel.meshCount != 0) UnloadModel(m_HandModel);
    if (m_ImpactModel.meshCount != 0) UnloadModel(m_ImpactModel);
    m_Lightning.Shutdown();
    if (IsShaderValid(m_LightingShader)) UnloadShader(m_LightingShader);
    if (IsShaderValid(m_ShadowShader)) UnloadShader(m_ShadowShader);
    if (m_pWeapon) {
        m_pWeapon->Shutdown();
        delete m_pWeapon;
        m_pWeapon = nullptr;
    }
    gs_Resources.UnloadAll();
}

} // namespace game
