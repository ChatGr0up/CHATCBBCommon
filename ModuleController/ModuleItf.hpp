#pragma once
#include <string>
#include <ModuleFactory.hpp>
#define REGISTER_MODULE(ModuleClass) \
namespace { \
    static bool _registered_##ModuleClass = [] { \
        CBB::ModuleController::ModuleFactory::registerModule( \
            #ModuleClass, [] { return std::make_unique<ModuleClass>(); } \
        ); \
        return true; \
    }(); \
}

namespace CBB::ModuleController {
class ModuleItf {
public:
    virtual ~ModuleItf() = default;
    virtual bool init() = 0;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual std::string name() const = 0;
protected:
    ModuleItf() = default;
};
}
