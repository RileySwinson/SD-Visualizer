#pragma once

#include "../spatial/sdtreeSpatial.h"
#include "../directional/dirKind.h"

#include <Eigen/Dense>

#include <vector>

// Find the smallest-AABB spatial leaf containing `point`. If outside all
// leaves, returns the index of the nearest leaf by centroid distance.
/**
 * @brief Find smallelst AABB spatial leaf containing @c point . If outside all leaves, return index of nearest leaft by centroid distance.
 * 
 * @param tree 
 * @param point 
 * @return int 
 */
int findNode(const SDTree& tree, const Eigen::Vector3f& point);

/**
 * @brief Smallest directional leaft at uv coordinate
 * 
 * @param leaves 
 * @param u 
 * @param v 
 * @return const DirLeaf* 
 */
const DirLeaf* findLeafUV(const std::vector<DirLeaf>& leaves, float u, float v);

// All leaves of `newerTree` whose AABB is fully contained within the
// (parentPos, parentSize) AABB. Assumes refinement (no spatial pruning).

/**
 * @brief All leaves of @c newerTree whose AABB is fully contained withing the ( @p parentPos, @p parentSize )
 * 
 * @param newerTree 
 * @param parentPos 
 * @param parentSize 
 * @return std::vector<int> 
 */
std::vector<int> findDescendants(
    const SDTree& newerTree,
    const Eigen::Vector3f& parentPos,
    const Eigen::Vector3f& parentSize
);
