#pragma once

#include "../spatial/sdtreeSpatial.h"

#include <memory>
#include <string>

/**
 * @brief Probe the dump thenispatch to the right spatial loader, and return the populated structure.
 * @param fn Path to the dump file.
 * @return The loaded SDTree on success, or @c nullptr on failure
 */
std::unique_ptr<SDTree> dispatchLoad(const std::string& fn);

/**
 * @brief Register all built-in loaders, directionla and spatial.
 *
 * Call once at startup, before any thread queries the registries.
 */
void registerAllLoaders();
