#pragma once

#include "ModuleItf.hpp"
#include <functional>
#include <memory>
#include <unordered_map>

namespace CBB::ModuleController {
class ModuleItf;
class ModuleFactory {
public:
    using ModuleCreator = std::function<std::unique_ptr<ModuleItf>()>;

    static void registerModule(const std::string& name, ModuleCreator creator) {
        getRegistry()[name] = std::move(creator);
    }

    static std::unique_ptr<ModuleItf> create(const std::string& name) {
        auto& reg = getRegistry();
        const auto& it = reg.find(name);
        if (it != reg.end()) {
            return (it->second)();
        }
        return nullptr;
    }

private:
    static std::unordered_map<std::string, ModuleCreator>& getRegistry() {
        static std::unordered_map<std::string, ModuleCreator> registry;
        return registry;
    }
};
} // namespace CBB::ModuleController