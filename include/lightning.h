#ifndef LIGHTNING_H
#define LIGHTNING_H

#include "scene_loader.h"

namespace game {

class Lightning {
public:
    void Initialize(const qscene::Scene& scene);
    void RenderShadowPass(const qscene::Scene& scene, qc::Model& primitiveModel, qc::Shader& shadowShader);
    void ApplyLighting(qc::Shader& shader, const qc::Camera3D& camera) const;
    void DrawDebugLights(const qscene::Scene& scene) const;
    void PrepareModel(qc::Model& model, qc::Shader& shader) const;
    void Shutdown();

private:
    std::array<qscene::SceneLight, qscene::MAX_SCENE_LIGHTS> m_aLights{};
    std::array<qc::RenderTexture2D, qscene::MAX_SCENE_LIGHTS> m_aShadowMaps{};
    std::array<qc::Camera3D, qscene::MAX_SCENE_LIGHTS> m_aShadowCameras{};
    std::array<qc::Matrix, qscene::MAX_SCENE_LIGHTS> m_aLightViews{};
    std::array<qc::Matrix, qscene::MAX_SCENE_LIGHTS> m_aLightProjections{};
    int m_LightCount = 0;
};

} // namespace game

#endif // LIGHTNING_H
