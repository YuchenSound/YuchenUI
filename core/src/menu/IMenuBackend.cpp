#include "YuchenUI/menu/IMenuBackend.h"

namespace YuchenUI {

static MenuBackendFactory& getFactoryStorage() {
    static MenuBackendFactory factory = nullptr;
    return factory;
}

void IMenuBackend::registerFactory(MenuBackendFactory factory) {
    getFactoryStorage() = factory;
}

std::unique_ptr<IMenuBackend> IMenuBackend::createBackend() {
    auto& factory = getFactoryStorage();
    if (factory) {
        return factory();
    }
    return nullptr;
}

}
