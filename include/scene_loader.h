#ifndef SCENE_LOADER_H
#define SCENE_LOADER_H

#include <array>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "QuarkCore/QuarkCore.hpp"
#include "nlohmann/json.hpp"

namespace qscene {

using Json = nlohmann::json;

struct Transform {
    qc::Vec3 position;
    qc::Vec3 rotation;
    qc::Vec3 scale{1.0f, 1.0f, 1.0f};
};

struct MeshComponent {
    bool enabled = true;
    std::string asset_name;
    int type = 0;
    bool is_primitive = true;
    bool is_editable_mesh = false;
    std::vector<qc::Vec3> editable_vertices;
    std::vector<qc::Vec2> editable_texcoords;
    std::vector<unsigned short> editable_indices;
};

struct MaterialComponent {
    bool enabled = true;
    qc::Color color{255, 255, 255, 255};
    qc::Color outline_color{200, 200, 200, 255};
    std::string texture_name, metallic_texture_name, normal_texture_name, roughness_texture_name;
    int texture_source = 0;
    bool auto_uv = false, texture_stretch = true;
    float repeat_u = 1.0f, repeat_v = 1.0f;
    qc::Vec2 uv_scale{1.0f, 1.0f};
    std::vector<std::string> material_slot_sources;
};
enum class LightType : int {
    Directional = 0,
    Point = 1,
    Spot = 2,
    Area = 3
};

struct LightComponent {
    bool enabled = true;
    LightType light_type = LightType::Directional;
    std::string light_color_hex;
    float light_intensity = 1.0f, light_range = 10.0f, light_spot_angle = 45.0f;
    qc::Vec3 light_position, light_rotation, light_target;
};

struct UnknownComponent {
    std::string type;
    Json raw;
};

struct Entity {
    std::string name;
    std::vector<std::string> tags;
    int parent_id = -1;
    bool is_group = false;
    Transform transform;
    std::optional<MeshComponent> mesh;
    std::optional<MaterialComponent> material;
    std::optional<LightComponent> light;
    std::vector<UnknownComponent> unknown_components;
};

struct Scene {
    std::string version;
    std::vector<Entity> entities;
};

inline constexpr int MAX_SCENE_LIGHTS = 4;
struct SceneLight {
    bool enabled = false;
    int type = 0;
    qc::Vec3 position{}, target{};
    qc::Vec4 color{1.0f, 1.0f, 1.0f, 1.0f};
    float intensity = 1.0f, range = 10.0f, spotAngle = 45.0f;
};

class SceneLoader {
public:
    Scene LoadFromFile(const std::string& path) const;
    static Scene ParseFile(const std::string& path);
    static qc::Matrix BuildTransformMatrix(const Transform& transform);
    static qc::Matrix BuildWorldTransformMatrix(const Scene& scene, int entityIndex);
    static std::string RemapAssetPath(const std::string& original);
    static qc::Vec4 ParseLightColor(const std::string& hex);
    static qc::Texture2D LoadMaterialTexture(const std::string& authoredPath);
    static void ApplySceneMaterial(qc::Model& model, const MaterialComponent& source);
    static void SetModelShader(qc::Model& model, qc::Shader* shader);
    static void SetModelShadowMaps(qc::Model& model,
        const std::array<qc::RenderTexture2D, MAX_SCENE_LIGHTS>& shadowMaps, int lightCount);
    static qc::Matrix CopyMatrix(const float* source);
    static void SetLightUniforms(const qc::Shader& shader,
        const std::array<SceneLight, MAX_SCENE_LIGHTS>& lights);
    static void SetShaderInt(const qc::Shader& shader, const char* name, int value);
    static void SetShaderFloat(const qc::Shader& shader, const char* name, float value);
    static void SetShaderVec3(const qc::Shader& shader, const char* name, const qc::Vec3& value);
    static void SetShaderVec4(const qc::Shader& shader, const char* name, const qc::Vec4& value);
};

} // namespace qscene

#endif // SCENE_LOADER_H
