#ifndef GAME_SCENE_H
#define GAME_SCENE_H

#include "lightning.h"
#include "player.h"

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
    qscene::Scene* m_pScene = nullptr;
    qc::Shader m_LightingShader{};
    qc::Shader m_ShadowShader{};
    qc::Model m_PrimitiveModel{};
    std::vector<std::string> m_vModelPaths;
    Player m_Player{{0.0f, 0.0f, 0.0f}};
    Lightning m_Lightning;
};

} // namespace game

#endif // GAME_SCENE_H
