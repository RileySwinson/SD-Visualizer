#pragma once

#include "../directional/iDirectional.h"

#include <Eigen/Dense>

#include <memory>
#include <string>
#include <vector>

/**
 * @brief A flattened spatial leaf. Common across all spatial structures.
 *
 * Each leaf points to a directional cache that lives at that leaf.
 */
struct SpatialNode {
    int index;
    Eigen::Vector3f pos;
    Eigen::Vector3f size;
    float meanRadiance;
    std::size_t nSamples;
    std::shared_ptr<IDirectional> dTree;
};

/**
 * @brief Interface for any spatial structure (SD-Tree, kd-tree, grid, BVH, ...).
 *
 * All structures expose a flattened leaf list, scene AABB, and the directional kind of the caches at the leaves.
 *
 * Concrete structures may expose additional internals (e.g. SDTree's binary tree, used for inter-iteration structure comparison). Such operations are resolved at the comparison site, not on this interface.
 */
class ISpatial {
public:
    virtual ~ISpatial() = default;

    /// @brief Identifier shown in the UI.
    virtual const std::string& getName() const = 0;

    /// @brief Sequence-group index, or @c -1 if standalone.
    virtual int getGroupIndex() const = 0;

    /// @brief Directional kind stored at the leaves of this structure.
    virtual DirKind getDirectionalType() const = 0;

    /// @brief Minimum corner of the scene AABB.
    virtual const Eigen::Vector3f& getAabbMin() const = 0;

    /// @brief Maximum corner of the scene AABB.
    virtual const Eigen::Vector3f& getAabbMax() const = 0;

    /// @brief Flat list of spatial leaves.
    virtual const std::vector<SpatialNode>& getNodes() const = 0;
};
