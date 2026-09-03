#include "lightning.h"

using namespace qc;

#include "resources.h"

namespace game {

void Lightning::Initialize(const qscene::Scene& scene) {
    m_LightCount = 0;
    m_aLights = {};

    for (const auto& entity : scene.entities) {
        if (!entity.light || !entity.light->enabled || m_LightCount >= qscene::MAX_SCENE_LIGHTS) continue;

        const auto& source = *entity.light;
        auto& destination = m_aLights[m_LightCount++];
        destination.enabled = true;
        destination.type = static_cast<int>(source.light_type);
        destination.position = entity.transform.position;
        destination.target = source.light_target;
        destination.color = qscene::SceneLoader::ParseLightColor(source.light_color_hex);
        destination.intensity = source.light_intensity;
        destination.range = source.light_range;
        destination.spotAngle = source.light_spot_angle;
    }

    for (int i = 0; i < m_LightCount; ++i) {
        m_aShadowMaps[i] = LoadRenderTexture(1024, 1024);
        m_aShadowCameras[i] = Camera3D{};
        m_aShadowCameras[i].position = m_aLights[i].position;
        m_aShadowCameras[i].target = m_aLights[i].target;
        m_aShadowCameras[i].up = Vec3{0.0f, 1.0f, 0.0f};
        m_aShadowCameras[i].fovy = 55.0f;
        m_aShadowCameras[i].projection = CAMERA_PERSPECTIVE;
    }
}

void Lightning::RenderShadowPass(const qscene::Scene& scene, Model& primitiveModel, Shader& shadowShader) {
    for (int i = 0; i < m_LightCount; ++i) {
        BeginTextureMode(m_aShadowMaps[i]);
        ClearBackground(WHITE);
        BeginMode3D(m_aShadowCameras[i]);

        m_aLightViews[i] = qscene::SceneLoader::CopyMatrix(GetMatrixModelview());
        m_aLightProjections[i] = qscene::SceneLoader::CopyMatrix(GetMatrixProjection());

        for (const auto& entity : scene.entities) {
            if (!entity.mesh || !entity.mesh->enabled) continue;

            if (entity.mesh->is_primitive) {
                primitiveModel.transform = qscene::SceneLoader::BuildTransformMatrix(entity.transform);
                qscene::SceneLoader::SetModelShader(primitiveModel, &shadowShader);
                DrawModel(primitiveModel, Vec3{0.0f, 0.0f, 0.0f}, 1.0f, WHITE);
            } else {
                std::string path = qscene::SceneLoader::RemapAssetPath(entity.mesh->asset_name);
                if (gs_Resources.Has<Model>(path)) {
                    Model& model = gs_Resources.Get<Model>(path);
                    model.transform = qscene::SceneLoader::BuildTransformMatrix(entity.transform);
                    qscene::SceneLoader::SetModelShader(model, &shadowShader);
                    DrawModel(model, Vec3{0.0f, 0.0f, 0.0f}, 1.0f, WHITE);
                }
            }
        }

        EndMode3D();
        EndTextureMode();
    }
}

void Lightning::ApplyLighting(Shader& shader, const Camera3D& camera) const {
    qscene::SceneLoader::SetShaderVec3(shader, "viewPos", camera.position);
    qscene::SceneLoader::SetShaderVec4(shader, "ambient", Vec4{0.025f, 0.025f, 0.025f, 1.0f});
    qscene::SceneLoader::SetShaderVec3(shader, "emissionColor", Vec3{0.0f, 0.0f, 0.0f});
    qscene::SceneLoader::SetShaderFloat(shader, "emissionPower", 0.0f);
    qscene::SceneLoader::SetShaderInt(shader, "shadowsEnabled", m_LightCount > 0 ? 1 : 0);
    qscene::SceneLoader::SetShaderFloat(shader, "shadowBias", 0.0025f);
    qscene::SceneLoader::SetShaderInt(shader, "shadowFilterQuality", 1);
    qscene::SceneLoader::SetLightUniforms(shader, m_aLights);

    for (int i = 0; i < m_LightCount; ++i) {
        SetShaderValueMatrix(shader,
            GetShaderLocation(shader, TextFormat("lightViews[%i]", i)), m_aLightViews[i]);
        SetShaderValueMatrix(shader,
            GetShaderLocation(shader, TextFormat("lightProjections[%i]", i)), m_aLightProjections[i]);
    }
}

void Lightning::DrawDebugLights(const qscene::Scene& scene) const {
    for (const auto& entity : scene.entities) {
        if (entity.light && entity.light->enabled)
            DrawSphere(entity.light->light_position, 0.15f, YELLOW);
    }
}

void Lightning::PrepareModel(Model& model, Shader& shader) const {
    qscene::SceneLoader::SetModelShader(model, &shader);
    qscene::SceneLoader::SetModelShadowMaps(model, m_aShadowMaps, m_LightCount);
}

void Lightning::Shutdown() {
    for (int i = 0; i < m_LightCount; ++i) UnloadRenderTexture(m_aShadowMaps[i]);
    m_LightCount = 0;
}

} // namespace game
