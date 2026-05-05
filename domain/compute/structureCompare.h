#pragma once

#include "../spatial/sdtreeSpatial.h"

#include <vector>

/**
 * @brief Per-leaf division status when comparing two iterations of a spatial structure.
 */
enum DivStatus { DIV_UNCHANGED = 0, DIV_REFINED = 1, DIV_DIVERGENT = 2 };

/**
 * @brief Result of comparing two SD-trees: per-leaf @ref DivStatus plus
 * summary counts.
 */
struct DivisionResult {
    std::vector<DivStatus> status;       ///< Per-leaf status of the newer tree.
    int  numUnchanged = 0;               ///< Count of @c DIV_UNCHANGED leaves.
    int  numRefined   = 0;               ///< Count of @c DIV_REFINED leaves.
    int  numDivergent = 0;               ///< Count of @c DIV_DIVERGENT leaves.
    bool valid        = false;           ///< @c false if compare wasn't run.
};

/**
 * @brief Compare two SD-trees that should be refinements of each other
 * (subsequent iterations of the same render).
 *
 * @param older
 * @param newer
 * @return DivisionResult Per-leaf status of @p newer plus summary counts.
 */
DivisionResult compareStructure(const SDTree& older, const SDTree& newer);
