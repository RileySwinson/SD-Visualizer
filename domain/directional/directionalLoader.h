#pragma once

#include "iDirectional.h"

#include <functional>
#include <memory>
#include <string>

/// Reads a directional distribution from the current dump and returns it as IDirectional. 
using DirectionalLoaderFn = std::function<std::unique_ptr<IDirectional>(BlobReader&)>;
// this is type erasure

/**
 * @brief Singleton registery mapping @ref DirType values to loader functions
 * 
 * Populated at program startup by @ref registerBuiltinDirectionalLoaders(). 
 * Registration is deterministic as all registration is explicit.
 * 
 * @note This is not thread-safe. Register all directional structures before using the registery.
 */
class DirectionalLoaderRegistry {
public:
    /// @brief Access the global singleton instance.
    static DirectionalLoaderRegistry& instance();

    /**
     * @brief Define @p fn as the loader for @p kind . This replaces any existing entry.
     * 
     * @param kind Directional distribution type to register a loader for.
     * @param fn   The loader callback that gets moved into the registry.
     */
    void registerLoader(DirKind kind, DirectionalLoaderFn fn);

    /**
     * @brief Look up the loader for @p kind.
     * 
     * @param kind Directional distribution type.
     * 
     * @return The registered loader, or an empty @c std::function if @p kind is out of range or no loader has been registered for it.
     */
    DirectionalLoaderFn lookup(DirKind kind) const;

    /// @brief @c true iff a loader has been registered for @p kind.
    bool has(DirKind kind) const;

private:
    /// Indexed by @ref DirType 
    DirectionalLoaderFn mLoaders[3] = {}; 
};

/// Invoke the registered loader for kind against the current reader.
std::unique_ptr<IDirectional> loadDirectional(BlobReader& b, DirKind kind);

/**
 * @brief Install the built-in directional structure loaders into the global registry.
 *
 * Call once during program startup before any queries.
 */
void registerBuiltinDirectionalLoaders();
