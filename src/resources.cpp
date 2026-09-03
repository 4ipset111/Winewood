#include "resources.h"
#include <type_traits>

using namespace qc;

ResourceManager gs_Resources;

template<typename T>
void ResourceManager::Storage<T>::UnloadAll() {
    resources.clear();
}

template<typename T>
ResourceManager::Storage<T>& ResourceManager::GetStorage()
{
    std::type_index type = typeid(T);
    auto it = storages.find(type);

    if (it == storages.end())
    {
        auto storage = std::make_unique<Storage<T>>();

        Storage<T>* ptr = storage.get();
        storages.emplace(type, std::move(storage));

        return *ptr;
    }

    return *static_cast<Storage<T>*>(it->second.get());
}

template<>
void ResourceManager::Load<Texture2D>(const std::string& name, const std::string& path) {
    ResourceManager::Storage<Texture2D>& storage = GetStorage<Texture2D>();

    if (storage.resources.find(name) != storage.resources.end()) {
        return;
    }

    Texture2D texture = ::LoadTexture(path.c_str());

    if (texture.id == 0) {
        throw std::runtime_error("Failed to load texture: " + path);
    }

    storage.resources.emplace(name, texture);
}

template<>
void ResourceManager::Load<Sound>(const std::string& name, const std::string& path) {
    ResourceManager::Storage<Sound>& storage = GetStorage<Sound>();

    if (storage.resources.find(name) != storage.resources.end()) {
        return;
    }

    Sound sound = ::LoadSound(path.c_str());

    if (sound.frameCount == 0) {
        throw std::runtime_error("Failed to load sound: " + path);
    }

    storage.resources.emplace(name, sound);
}

template<>
void ResourceManager::Load<Music>(const std::string& name, const std::string& path) {
    ResourceManager::Storage<Music>& storage = GetStorage<Music>();

    if (storage.resources.find(name) != storage.resources.end())
        return;

    Music music = ::LoadMusicStream(path.c_str());

    if (music.frameCount == 0) {
        throw std::runtime_error("Failed to load music: " + path);
    }

    storage.resources.emplace(name, music);
}

template<>
void ResourceManager::Load<Font>(const std::string& name, const std::string& path) {
    ResourceManager::Storage<Font>& storage = GetStorage<Font>();

    if (storage.resources.find(name) != storage.resources.end()) {
        return;
    }

    Font font = ::LoadFont(path.c_str());

    if (font.texture.id == 0) {
        throw std::runtime_error("Failed to load font: " + path);
    }

    storage.resources.emplace(name, font);
}

template<>
void ResourceManager::Load<qscene::Scene>(const std::string& name, const std::string& path) {
    ResourceManager::Storage<qscene::Scene>& storage = GetStorage<qscene::Scene>();

    if (storage.resources.find(name) != storage.resources.end()) {
        return;
    }

    qscene::Scene scene = qscene::SceneLoader::ParseFile(path);
    storage.resources.emplace(name, std::move(scene));
}

template<>
void ResourceManager::Load<Model>(const std::string& name, const std::string& path) {
    ResourceManager::Storage<Model>& storage = GetStorage<Model>();

    if (storage.resources.find(name) != storage.resources.end()) {
        return;
    }

    Model model = ::LoadModel(path.c_str());
    if (model.meshCount == 0) {
        throw std::runtime_error("Failed to load model: " + path);
    }
    storage.resources.emplace(name, model);
}


template<typename T>
T& ResourceManager::Get(const std::string& name) {
    ResourceManager::Storage<T>& storage = GetStorage<T>();

    auto it = storage.resources.find(name);
    if (it == storage.resources.end())
    {
        throw std::runtime_error("Resource not found: " + name);
    }

    return it->second;
}

template<typename T>
bool ResourceManager::Has(const std::string& name) const {
    auto it = storages.find(typeid(T));

    if (it == storages.end())
        return false;

    auto* storage = static_cast<const Storage<T>*>(it->second.get());

    return storage->resources.contains(name);
}

template<>
void ResourceManager::Unload<Texture2D>(const std::string& name) {
    ResourceManager::Storage<Texture2D>& storage = GetStorage<Texture2D>();

    auto it = storage.resources.find(name);

    if (it != storage.resources.end())
    {
        ::UnloadTexture(it->second);
        storage.resources.erase(it);
    }
}


template<>
void ResourceManager::Unload<Sound>(const std::string& name) {
    ResourceManager::Storage<Sound>& storage = GetStorage<Sound>();

    auto it = storage.resources.find(name);
    if (it != storage.resources.end())
    {
        ::UnloadSound(it->second);
        storage.resources.erase(it);
    }
}

template<>
void ResourceManager::Unload<Music>(const std::string& name) {
    ResourceManager::Storage<Music>& storage = GetStorage<Music>();

    auto it = storage.resources.find(name);
    if (it != storage.resources.end())
    {
        ::UnloadMusicStream(it->second);
        storage.resources.erase(it);
    }
}

template<>
void ResourceManager::Unload<Font>(const std::string& name) {
    ResourceManager::Storage<Font>& storage = GetStorage<Font>();

    auto it = storage.resources.find(name);
    if (it != storage.resources.end())
    {
        ::UnloadFont(it->second);
        storage.resources.erase(it);
    }
}

template<>
void ResourceManager::Unload<Model>(const std::string& name) {
    ResourceManager::Storage<Model>& storage = GetStorage<Model>();

    auto it = storage.resources.find(name);
    if (it != storage.resources.end()) {
        ::UnloadModel(it->second);
        storage.resources.erase(it);
    }
}

template<>
void ResourceManager::Storage<Texture2D>::UnloadAll() {
    for (auto& [name, texture] : resources)
        ::UnloadTexture(texture);

    resources.clear();
}


template<>
void ResourceManager::Storage<Sound>::UnloadAll() {
    for (auto& [name, sound] : resources)
        ::UnloadSound(sound);

    resources.clear();
}


template<>
void ResourceManager::Storage<Music>::UnloadAll() {
    for (auto& [name, music] : resources)
        ::UnloadMusicStream(music);

    resources.clear();
}


template<>
void ResourceManager::Storage<Font>::UnloadAll() {
    for (auto& [name, font] : resources)
        ::UnloadFont(font);

    resources.clear();
}

template<>
void ResourceManager::Storage<Model>::UnloadAll() {
    for (auto& [name, model] : resources)
        ::UnloadModel(model);

    resources.clear();
}


void ResourceManager::UnloadAll() {
    for (auto& [type, storage] : storages)
        storage->UnloadAll();

    storages.clear();
}

template Texture2D& ResourceManager::Get<Texture2D>(const std::string&);
template Sound& ResourceManager::Get<Sound>(const std::string&);
template Music& ResourceManager::Get<Music>(const std::string&);
template Font& ResourceManager::Get<Font>(const std::string&);
template qscene::Scene& ResourceManager::Get<qscene::Scene>(const std::string&);
template Model& ResourceManager::Get<Model>(const std::string&);

template bool ResourceManager::Has<Texture2D>(const std::string&) const;
template bool ResourceManager::Has<Sound>(const std::string&) const;
template bool ResourceManager::Has<Music>(const std::string&) const;
template bool ResourceManager::Has<Font>(const std::string&) const;
template bool ResourceManager::Has<Model>(const std::string&) const;