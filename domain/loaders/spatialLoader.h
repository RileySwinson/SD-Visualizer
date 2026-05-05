#pragma once

#include "dumpDescriptor.h"
#include "../spatial/sdtreeSpatial.h"

#include <functional>
#include <memory>
#include <optional>
#include <string>

/**
 * @brief Loader callback. Reads a dump file into a concrete spatial structure (SDTree for now).
 *
 * Loaders trust the descriptor produced by the probe stage.
 *
 * Concrete return type is currently @c SDTree because the rest of the visualizer reaches into SDTree-specific fields. When a second spatial type is added, migrate this to @c std::unique_ptr<ISpatial> .
 */
using SpatialLoaderFn = std::function<std::unique_ptr<SDTree>(const std::string& fn, const DumpDescriptor& desc)>;

/**
 * @brief Registry mapping @ref SpatialKind values to loader callbacks.
 *
 * Populated at program startup by @ref registerBuiltinSpatialLoaders().
 *
 * @note Not thread-safe. Register all loaders before any worker threads start querying the registry.
 * @see registerBuiltinSpatialLoaders
 */
class SpatialLoaderRegistry {
public:
    /// @brief Access the global singleton instance.
    static SpatialLoaderRegistry& instance();

    /**
     * @brief Install @p fn as the loader for @p kind . Replaces any existing entry.
     * @param kind Spatial structure type to register a loader for.
     * @param fn   Loader callback; moved into the registry.
     */
    void registerLoader(SpatialKind kind, SpatialLoaderFn fn);

    /**
     * @brief Look up the loader for @p kind .
     * @param kind Spatial structure type.
     * @return The registered loader, or an empty @c std::function if no loader is registered for @p kind .
     */
    SpatialLoaderFn lookup(SpatialKind kind) const;

    /// @brief @c true iff a loader has been registered for @p kind .
    bool has(SpatialKind kind) const;

private:
    SpatialLoaderFn mLoaders[1] = {}; ///< Indexed by @ref SpatialKind . Currently only SDTree.
};

/**
 * @brief Register the built-in spatial loaders.
 *
 * Call once at startup, before any thread queries the registry.
 */
void registerBuiltinSpatialLoaders();
