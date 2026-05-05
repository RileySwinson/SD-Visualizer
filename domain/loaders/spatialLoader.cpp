#include "spatialLoader.h"

#include <stdexcept>

SpatialLoaderRegistry& SpatialLoaderRegistry::instance() {
    static SpatialLoaderRegistry r;
    return r;
}

void SpatialLoaderRegistry::registerLoader(SpatialKind kind, SpatialLoaderFn fn) {
    int slot = (int)kind;
    if (slot < 0 || slot > 0) throw std::out_of_range("SpatialKind out of range");
    mLoaders[slot] = std::move(fn);
}

SpatialLoaderFn SpatialLoaderRegistry::lookup(SpatialKind kind) const {
    int slot = (int)kind;
    if (slot < 0 || slot > 0) return {};
    return mLoaders[slot];
}

bool SpatialLoaderRegistry::has(SpatialKind kind) const {
    return static_cast<bool>(lookup(kind));
}

void registerBuiltinSpatialLoaders() {
    auto& reg = SpatialLoaderRegistry::instance();

    reg.registerLoader(SpatialKind::SDTree,
        [](const std::string& fn, const DumpDescriptor& desc) -> std::unique_ptr<SDTree> {
            auto t = std::make_unique<SDTree>();
            if (!t->load(fn, desc.dirKind)) return nullptr;
            return t;
        });
}
