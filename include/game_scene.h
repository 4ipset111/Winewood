#ifndef GAME_SCENE_H
#define GAME_SCENE_H

#include "lightning.h"
#include "player.h"
#include "weapon.h"
#include "inventory.h"

#include <string>
#include <unordered_map>
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
    std::unordered_map<int, qc::Model> m_vEditablePrimitiveModels;
    qc::Texture2D m_BulletImpactTexture{};
    std::vector<ImpactMark> m_vImpactMarks;
    std::vector<std::string> m_vModelPaths;
    Player m_Player{{0.0f, 0.0f, 0.0f}};
    IWeapon* m_pWeapon = nullptr;
    Lightning m_Lightning;
    Inventory m_Inventory;
};

} // namespace game

#endif // GAME_SCENE_H
