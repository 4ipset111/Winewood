#ifndef GAME_SCENE_H
#define GAME_SCENE_H

#include "lightning.h"
#include "player.h"

#include <array>
#include <string>
#include <vector>

namespace game {

class GameScene {
public:
    bool Initialize();
    void Update();
    void Draw();
    void Shutdown();

private:
    struct ImpactMark {
        qc::Vec3 position{};
        qc::Vec3 normal{};
        float lifetime = 0.0f;
    };

    qscene::Scene* m_pScene = nullptr;
    qc::Shader m_LightingShader{};
    qc::Shader m_ShadowShader{};
    qc::Model m_PrimitiveModel{};
    qc::Model m_HandModel{};
    qc::Model m_ImpactModel{};
    std::array<qc::Sound, 4> m_aShootSounds{};
    int m_ShootSoundIndex = 0;
    qc::Sound m_CurrentShootSound{};
    float m_CurrentShootVolume = 0.0f;
    float m_CurrentShootElapsed = 0.0f;
    float m_CurrentShootDuration = 0.0f;
    qc::Sound m_FadingShootSound{};
    float m_FadingShootVolume = 0.0f;
    bool m_ShootPending = false;
    float m_ShootDelay = 0.0f;
    qc::Texture2D m_BulletImpactTexture{};
    std::vector<ImpactMark> m_vImpactMarks;
    std::vector<std::string> m_vModelPaths;
    Player m_Player{{0.0f, 0.0f, 0.0f}};
    Lightning m_Lightning;
};

} // namespace game

#endif // GAME_SCENE_H
