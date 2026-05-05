#include "directionalLoader.h"

#include "btcTree.h"
#include "costQuadtreeTree.h"
#include "quadtreeTree.h"

#include <stdexcept>

DirectionalLoaderRegistry& DirectionalLoaderRegistry::instance() {
    static DirectionalLoaderRegistry registery; // singleton
    return registery;
}

void DirectionalLoaderRegistry::registerLoader(DirKind kind, DirectionalLoaderFn fn) {
    int slot = (int)kind;
    if (slot < 0 || slot >= DirKind::Count) throw std::out_of_range("DirType out of range");
    mLoaders[slot] = std::move(fn);
}

DirectionalLoaderFn DirectionalLoaderRegistry::lookup(DirKind kind) const {
    int slot = (int)kind;
    if (slot < 0 || slot >= DirKind::Count) return {};
    return mLoaders[slot];
}

bool DirectionalLoaderRegistry::has(DirKind kind) const {
    return static_cast<bool>(lookup(kind));
}

std::unique_ptr<IDirectional> loadDirectional(BlobReader& b, DirKind kind) {
    auto fn = DirectionalLoaderRegistry::instance().lookup(kind);
    if (!fn) return nullptr;
    return fn(b);
}

void registerBuiltinDirectionalLoaders() {
    auto& registery = DirectionalLoaderRegistry::instance();

    registery.registerLoader(DirKind::QuadTree, [](BlobReader& b) -> std::unique_ptr<IDirectional> {
        auto t = std::make_unique<QuadtreeTree>();
        if (!t->read(b)) return nullptr;
        return t;
    });

    registery.registerLoader(DirKind::BTC, [](BlobReader& b) -> std::unique_ptr<IDirectional> {
        auto t = std::make_unique<BtcTree>();
        if (!t->read(b)) return nullptr;
        return t;
    });

    registery.registerLoader(DirKind::CostQuadTree, [](BlobReader& b) -> std::unique_ptr<IDirectional> {
        auto t = std::make_unique<CostQuadtreeTree>();
        if (!t->read(b)) return nullptr;
        return t;
    });
}
