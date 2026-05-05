#include "dispatch.h"

#include "probe.h"
#include "spatialLoader.h"
#include "../directional/directionalLoader.h"

#include <iostream>

std::unique_ptr<SDTree> dispatchLoad(const std::string& fn) {
    DumpDescriptor desc = probeDump(fn);
    
    if (!desc.valid) {
        std::cerr << "Probe failed for: " << fn << "\n";
        return nullptr;
    }

    auto loader = SpatialLoaderRegistry::instance().lookup(desc.spatial);
    if (!loader) {
        std::cerr << "No spatial loader registered for kind " << (int)desc.spatial << "\n";
        return nullptr;
    }

    return loader(fn, desc);
}

void registerAllLoaders() {
    registerBuiltinDirectionalLoaders();
    registerBuiltinSpatialLoaders();
}
