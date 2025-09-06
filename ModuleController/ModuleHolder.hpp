#pragma once

#include <fstream>
#include <iostream>
#include <sstream>
#include "ModuleItf.hpp"
#include "boost/json.hpp"

namespace CBB::ModuleController {
class ModuleHolder {
public:
    static ModuleHolder& instance() {
        static ModuleHolder holder;
        return holder;
    }

    void init() {
        std::cout << "ModuleHolder inited" << std::endl;
    }

    ModuleItf& getModule(const std::string& moduleName) {
        if (!m_initialized) {
            registerModule();
        }
        for (const auto& module : m_modules) {
            if (module == nullptr) {
                continue;
            }
            if (module->name() == moduleName) {
                return *module;
            }
        }
        throw std::runtime_error("Module not found: " + moduleName);
    }

    void cleanup(int signal) {
        std::cout << "Cleaning up modules due to signal: " << signal << std::endl;
        for (auto& module : m_modules) {
            if (module) {
                try {
                    module->stop();
                } catch (const std::exception& e) {
                    std::cerr << "Error stopping module " + module->name() + ": " + std::string(e.what());
                }
            }
        }
        m_modules.clear();
    }
private:
    ModuleHolder() {
        registerModule();
    }

    ~ModuleHolder() = default;

    ModuleHolder(const ModuleHolder&) = delete;

    ModuleHolder& operator=(const ModuleHolder&) = delete;

    ModuleHolder(ModuleHolder&&) = delete;

    ModuleHolder& operator=(ModuleHolder&&) = delete;

    boost::json::value getJsonFromFile() {
        std::ifstream file("configs/module_config.json", std::ios::in);
        if (!file.is_open()) {
            std::cerr << "Failed to open module_config.json";
            return boost::json::value();
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        return boost::json::parse(buffer.str());
    }

    void registerModule() {
        try {
            if (m_initialized) {
                return;
            }
            boost::json::value jsonValue = getJsonFromFile();
            if (!jsonValue.is_object() || !jsonValue.as_object().contains("modules") ||
                !jsonValue.as_object()["modules"].is_array()) {
                std::cerr << "Invalid module_config.json format";
                return;
            }
            const auto& modulesArray = jsonValue.as_object()["modules"].as_array();
            for (const auto& moduleVal : modulesArray) {
                if (!moduleVal.is_object() || !moduleVal.as_object().contains("name") ||
                    !moduleVal.at("name").is_string()) {
                    std::cerr << "Invalid module entry in module_config.json";
                    continue;
                }
                std::string moduleName = moduleVal.at("name").as_string().c_str();
                try {
                    m_modules.emplace_back(ModuleFactory::create(moduleName));
                } catch (const std::exception& e) {
                    std::cerr << "Failed to load module: " + moduleName + ", error: " + std::string(e.what());
                }
            }
            m_initialized = true;
        } catch (const std::exception& e) {
            std::cerr << ("Failed to read module_config.json: " + std::string(e.what()));
        }
    }
private:
    std::vector<std::unique_ptr<ModuleItf>> m_modules;
    std::atomic_bool m_initialized{false};
};
} // namespace CBB::ModuleController