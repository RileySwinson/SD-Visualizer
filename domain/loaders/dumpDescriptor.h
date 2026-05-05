#pragma once

#include "../directional/dirKind.h"

#include <string>

/**
 * @brief Spatial structure kinds. Currently only SDTree.
 * 
 * Shoudl techncially be its own file but its one line so 
 */
enum class SpatialKind { SDTree };

/**
 * @brief The metadata result of probing a dump file. Currently uses filenames to discern dump types. Good enough for now.
 */
struct DumpDescriptor {
    SpatialKind spatial = SpatialKind::SDTree;  ///< Detected spatial structure kind.
    DirKind dirKind = DirKind::QuadTree;        ///< Detected directional kind at the leaves.
    bool valid = false;                         ///< @c true if the probe succeeded; otherwise the other fields are unspecified.
};
