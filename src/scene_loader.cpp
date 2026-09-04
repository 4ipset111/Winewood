#include "scene_loader.h"

using namespace qc;

#include "resources.h"

#include <fstream>
#include <stdexcept>

namespace qscene {
namespace {

Vec3 parseVec3(const Json& value, Vec3 fallback = {}) {
    if (!value.is_array() || value.size() < 3) return fallback;
    return Vec3{value[0].get<float>(), value[1].get<float>(), value[2].get<float>()};
}

Vec2 parseVec2(const Json& value, Vec2 fallback = {}) {
    if (!value.is_array() || value.size() < 2) return fallback;
    return Vec2{value[0].get<float>(), value[1].get<float>()};
}

Color parseColor(const Json& value, Color fallback = {}) {
    if (!value.is_array() || value.size() < 4) return fallback;
    return Color{(unsigned char)value[0].get<int>(), (unsigned char)value[1].get<int>(),
        (unsigned char)value[2].get<int>(), (unsigned char)value[3].get<int>()};
}

bool hasFileExtension(const std::string& value) {
    auto slash = value.find_last_of("/\\");
    auto dot = value.find_last_of('.');
    return dot != std::string::npos && (slash == std::string::npos || dot > slash);
}

LightType parseLightType(const Json& value) {
    int type = value.is_number_integer() ? value.get<int>() : 0;
    return type >= 0 && type <= 3 ? static_cast<LightType>(type) : LightType::Directional;
}

Entity parseEntity(const Json& value) {
    Entity entity;
    entity.name = value.value("name", "");
    entity.parent_id = value.value("parent_id", -1);
    entity.is_group = value.value("is_group", false);
    if (!value.contains("components")) return entity;

    for (const auto& component : value["components"]) {
        std::string type = component.value("type", "");
        bool enabled = component.value("enabled", true);
        const Json& data = component["data"];
        if (type == "Transform") {
            entity.transform.position = parseVec3(data["position"]);
            entity.transform.rotation = parseVec3(data["rotation"]);
            entity.transform.scale = parseVec3(data["scale"], Vec3{1, 1, 1});
        } else if (type == "Mesh") {
            MeshComponent mesh;
            mesh.enabled = enabled;
            mesh.asset_name = data.value("asset_name", "");
            mesh.type = data.value("type", 0);
            mesh.is_primitive = !hasFileExtension(mesh.asset_name);
            mesh.is_editable_mesh = data.value("is_editable_mesh", false);
            if (data.contains("editable_vertices") && data["editable_vertices"].is_array()) {
                for (const auto& vertex : data["editable_vertices"]) {
                    if (!vertex.is_array() || vertex.size() < 3) continue;
                    mesh.editable_vertices.push_back(Vec3{vertex[0].get<float>(), vertex[1].get<float>(), vertex[2].get<float>()});
                    mesh.editable_texcoords.push_back(vertex.size() >= 5
                        ? Vec2{vertex[3].get<float>(), vertex[4].get<float>()} : Vec2{});
                }
            }
            if (data.contains("editable_triangles") && data["editable_triangles"].is_array()) {
                for (const auto& triangle : data["editable_triangles"]) {
                    if (!triangle.is_array() || triangle.size() < 3) continue;
                    for (int index = 0; index < 3; ++index)
                        mesh.editable_indices.push_back(triangle[index].get<unsigned short>());
                }
            }
            entity.mesh = mesh;
        } else if (type == "Material") {
            MaterialComponent material;
            material.enabled = enabled;
            material.color = parseColor(data["color"]);
            material.outline_color = parseColor(data["outline_color"]);
            material.texture_name = data.value("texture_name", "");
            if (material.texture_name.empty())
                material.texture_name = data.value("albedo_texture_name", "");
            material.metallic_texture_name = data.value("metallic_texture_name", "");
            material.normal_texture_name = data.value("normal_texture_name", "");
            material.roughness_texture_name = data.value("roughness_texture_name", "");
            material.texture_source = data.value("texture_source", 0);
            material.auto_uv = data.value("auto_uv", false);
            material.texture_stretch = data.value("texture_stretch", true);
            material.repeat_u = data.value("repeat_u", 1.0f);
            material.repeat_v = data.value("repeat_v", 1.0f);
            material.uv_scale = parseVec2(data["uv_scale"], Vec2{1, 1});
            if (data.contains("material_slot_sources"))
                for (const auto& source : data["material_slot_sources"])
                    material.material_slot_sources.push_back(source.get<std::string>());
            entity.material = material;
        } else if (type == "Light") {
            LightComponent light;
            light.enabled = enabled && data.value("light_enabled", true);
            light.light_type = parseLightType(data.value("light_type", 0));
            light.light_color_hex = data.value("light_color", "FFFFFFFF");
            light.light_intensity = data.value("light_intensity", 1.0f);
            light.light_range = data.value("light_range", 10.0f);
            light.light_spot_angle = data.value("light_spot_angle", 45.0f);
            light.light_position = parseVec3(data["light_position"]);
            light.light_rotation = parseVec3(data["light_rotation"]);
            light.light_target = parseVec3(data["light_target"]);
            entity.light = light;
        } else {
            entity.unknown_components.push_back(UnknownComponent{type, component});
        }
    }
    return entity;
}

} // namespace

Scene SceneLoader::LoadFromFile(const std::string& path) const {
    return ParseFile(path);
}

Scene SceneLoader::ParseFile(const std::string& path) {
    std::string resolvedPath = RemapAssetPath(path);
    std::ifstream file;
    file.open(resolvedPath);
    if (!file.is_open() && resolvedPath != path) {
        file.clear();
        file.open(path);
    }
    if (!file.is_open()) throw std::runtime_error("QuarkSceneLoader: could not open file: " + path);
    Json json;
    file >> json;
    Scene scene;
    scene.version = json.value("version", "");
    if (json.contains("entities"))
        for (const auto& entity : json["entities"]) scene.entities.push_back(parseEntity(entity));
    return scene;
}

Matrix SceneLoader::BuildTransformMatrix(const Transform& transform) {
    Matrix scale = MatrixScale(transform.scale.x, transform.scale.y, transform.scale.z);
    Matrix rotation = MatrixRotateXYZ(Vec3{transform.rotation.x * DEG2RAD,
        transform.rotation.y * DEG2RAD, transform.rotation.z * DEG2RAD});
    Matrix translation = MatrixTranslate(transform.position.x, transform.position.y, transform.position.z);
    return MatrixMultiply(MatrixMultiply(translation, rotation), scale);
}

Matrix SceneLoader::BuildWorldTransformMatrix(const Scene& scene, int entityIndex) {
    if (entityIndex < 0 || entityIndex >= static_cast<int>(scene.entities.size()))
        return MatrixIdentity();

    Matrix world = BuildTransformMatrix(scene.entities[entityIndex].transform);
    const int parentIndex = scene.entities[entityIndex].parent_id;
    if (parentIndex >= 0 && parentIndex < static_cast<int>(scene.entities.size()) && parentIndex != entityIndex)
        world = MatrixMultiply(BuildWorldTransformMatrix(scene, parentIndex), world);
    return world;
}

std::string SceneLoader::RemapAssetPath(const std::string& original) {
    if (original.empty()) return original;
    std::string normalized = original;
    for (char& character : normalized) if (character == '\\') character = '/';
    const auto resourcesPosition = normalized.find("resources");
    if (resourcesPosition != std::string::npos)
        normalized = normalized.substr(resourcesPosition);

    if (normalized.rfind("resources/", 0) == 0) {
        if (FileExists(normalized.c_str())) return normalized;

        std::string withoutResources = normalized.substr(std::string("resources/").size());
        if (FileExists(withoutResources.c_str())) return withoutResources;
        std::string sceneAssetPath = "resources/scenes/" + withoutResources;
        if (FileExists(sceneAssetPath.c_str())) return sceneAssetPath;
        return normalized;
    }

    std::string withResources = "resources/" + normalized;
    if (FileExists(withResources.c_str())) return withResources;
    if (FileExists(normalized.c_str())) return normalized;
    std::string sceneAssetPath = "resources/scenes/" + normalized;
    if (FileExists(sceneAssetPath.c_str())) return sceneAssetPath;
    return withResources;
}

Vec4 SceneLoader::ParseLightColor(const std::string& hex) {
    if (hex.size() != 8) return Vec4{1, 1, 1, 1};
    try {
        unsigned int value = std::stoul(hex, nullptr, 16);
        return Vec4{float((value >> 24) & 0xff) / 255.0f, float((value >> 16) & 0xff) / 255.0f,
            float((value >> 8) & 0xff) / 255.0f, float(value & 0xff) / 255.0f};
    } catch (...) { return Vec4{1, 1, 1, 1}; }
}

Texture2D SceneLoader::LoadMaterialTexture(const std::string& authoredPath) {
    if (authoredPath.empty()) return Texture2D{};
    auto extension = authoredPath.find_last_of('.');
    if (extension == std::string::npos) return Texture2D{};
    std::string suffix = authoredPath.substr(extension);
    for (char& character : suffix)
        if (character >= 'A' && character <= 'Z') character += 'a' - 'A';
    if (suffix != ".png" && suffix != ".jpg" && suffix != ".jpeg" && suffix != ".bmp" && suffix != ".tga" &&
        suffix != ".gif" && suffix != ".psd" && suffix != ".hdr" && suffix != ".qoi") return Texture2D{};
    std::string path = RemapAssetPath(authoredPath);
    if (!FileExists(path.c_str()) && FileExists(authoredPath.c_str())) path = authoredPath;
    if (!FileExists(path.c_str())) return Texture2D{};
    gs_Resources.Load<Texture2D>(path, path);
    return gs_Resources.Get<Texture2D>(path);
}

void SceneLoader::ApplySceneMaterial(Model& model, const MaterialComponent& source) {
    if (!model.materials) return;

    Texture2D albedo = LoadMaterialTexture(source.texture_name);
    Texture2D metallic = LoadMaterialTexture(source.metallic_texture_name);
    Texture2D normal = LoadMaterialTexture(source.normal_texture_name);
    Texture2D roughness = LoadMaterialTexture(source.roughness_texture_name);

    for (int i = 0; i < model.materialCount; ++i) {
        Material& material = model.materials[i];
        if (!material.maps) continue;
        
        material.maps[MATERIAL_MAP_ALBEDO].color = source.color;
        Texture2D slotTexture = albedo;
        if (i < static_cast<int>(source.material_slot_sources.size()) &&
            !source.material_slot_sources[i].empty())
            slotTexture = LoadMaterialTexture(source.material_slot_sources[i]);
        if (slotTexture.valid || source.texture_source != 2)
            material.maps[MATERIAL_MAP_ALBEDO].texture = slotTexture;
        if (metallic.valid) material.maps[MATERIAL_MAP_METALNESS].texture = metallic;
        if (normal.valid) material.maps[MATERIAL_MAP_NORMAL].texture = normal;
        if (roughness.valid) material.maps[MATERIAL_MAP_ROUGHNESS].texture = roughness;
    }
}

void SceneLoader::SetModelShader(Model& model, Shader* shader) {
    for (int i = 0; i < model.materialCount; ++i) model.materials[i].shader = shader;
}

void SceneLoader::SetModelShadowMaps(Model& model,
    const std::array<RenderTexture2D, MAX_SCENE_LIGHTS>& shadowMaps, int lightCount) {
    for (int i = 0; i < model.materialCount; ++i) {
        if (!model.materials[i].maps) continue;
        for (int lightIndex = 0; lightIndex < lightCount; ++lightIndex)
            model.materials[i].maps[MATERIAL_MAP_HEIGHT + lightIndex].texture = shadowMaps[lightIndex].texture;
    }
}

Matrix SceneLoader::CopyMatrix(const float* source) {
    Matrix result{};
    for (int i = 0; i < 16; ++i) result.m[i] = source[i];
    return result;
}

void SceneLoader::SetShaderInt(const Shader& shader, const char* name, int value) {
    int location = GetShaderLocation(shader, name);
    if (location >= 0) SetShaderValue(shader, location, value);
}
void SceneLoader::SetShaderFloat(const Shader& shader, const char* name, float value) {
    int location = GetShaderLocation(shader, name);
    if (location >= 0) SetShaderValue(shader, location, value);
}
void SceneLoader::SetShaderVec3(const Shader& shader, const char* name, const Vec3& value) {
    int location = GetShaderLocation(shader, name);
    if (location >= 0) SetShaderValue(shader, location, value);
}
void SceneLoader::SetShaderVec4(const Shader& shader, const char* name, const Vec4& value) {
    int location = GetShaderLocation(shader, name);
    if (location >= 0) SetShaderValue(shader, location, value);
}
void SceneLoader::SetLightUniforms(const Shader& shader,
    const std::array<SceneLight, MAX_SCENE_LIGHTS>& lights) {
    for (int i = 0; i < MAX_SCENE_LIGHTS; ++i) {
        std::string prefix = "lights[" + std::to_string(i) + "]";
        SetShaderInt(shader, (prefix + ".enabled").c_str(), lights[i].enabled ? 1 : 0);
        SetShaderInt(shader, (prefix + ".type").c_str(), lights[i].type);
        SetShaderVec3(shader, (prefix + ".position").c_str(), lights[i].position);
        SetShaderVec3(shader, (prefix + ".target").c_str(), lights[i].target);
        SetShaderVec4(shader, (prefix + ".color").c_str(), lights[i].color);
        SetShaderFloat(shader, (prefix + ".intensity").c_str(), lights[i].intensity);
        SetShaderFloat(shader, (prefix + ".range").c_str(), lights[i].range);
        SetShaderFloat(shader, (prefix + ".spotAngle").c_str(), lights[i].spotAngle);
    }
}

} // namespace qscene
