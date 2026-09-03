#ifndef RESOURCES_H
#define RESOURCES_H

#include "QuarkCore/QuarkCore.hpp"

#include <string>
#include <unordered_map>
#include <typeindex>
#include <memory>
#include <stdexcept>

class ResourceManager {
public:
    template<typename T>
    void Load(const std::string& name, const std::string& path);

    template<typename T>
    T& Get(const std::string& name);

    template<typename T>
    bool Has(const std::string& name) const;

    template<typename T>
    void Unload(const std::string& name);

    void UnloadAll();

private:
    struct IStorage {
        virtual ~IStorage() = default;
        virtual void UnloadAll() = 0;
    };

    template<typename T>
    struct Storage : IStorage {
        std::unordered_map<std::string, T> resources;
        void UnloadAll() override;
    };

    template<typename T>
    Storage<T>& GetStorage();

    std::unordered_map<std::type_index, std::unique_ptr<IStorage>> storages;
};

extern ResourceManager Resources;

#endif // RESOURCES_H