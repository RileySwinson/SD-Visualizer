#pragma once

#include "iSpatial.h"
#include "../blobReader.h"

#include <Eigen/Dense>

#include <memory>
#include <string>
#include <vector>

/**
 * @brief One node of the SD-Tree's binary spatial subdivision.
 *
 * Internal nodes split along @c axis
 */
struct STreeNode {
    bool isLeaf;
    int axis;
    uint32_t children[2];
    int leafIndex = -1;
    std::shared_ptr<IDirectional> dTree;

    /**
     * @brief Read this node from the current position in the dump.
     * @param b BlobReader positioned at the start of the node payload.
     * @param directionalType Directional kind to construct at this node if it is a leaf.
     * @return @c true on success, @c false otherwise.
     */
    bool read(BlobReader& b, DirKind directionalType);
};

/**
 * @brief SD-Tree spatial structure is a binary subdivision of the scene AABB, with a directional cache at each leaf.
 *
 * Public fields are kept directly accessible. The ISpatial interface methods are getters over those same fields.
 */
class SDTree : public ISpatial {
public:
    std::vector<SpatialNode> nodes; ///< Flattened leaves.
    std::vector<STreeNode> treeNodes; ///< Raw binary tree (used by structure compare).
    Eigen::Matrix4f cameraMatrix;
    std::string name;
    std::string header;
    Eigen::Vector3f aabbMin;
    Eigen::Vector3f aabbMax;
    DirKind type = QuadTree;
    int groupIndex = -1;

    /**
     * @brief Load an SD-Tree dump.
     * @param fn Path to the dump file.
     * @param directionalKind Directional kind detected by the probe stage.
     * @return @c true on success, @c false otherwise.
     */
    bool load(const std::string& fn, DirKind directionalKind);

    // ISpatial overrides

    /// @brief Identifier shown in the UI.
    const std::string& getName() const override { return name; }

    /// @brief Sequence-group index, or @c -1 if standalone.
    int getGroupIndex() const override { return groupIndex; }

    /// @brief Directional kind stored at the leaves of this tree.
    DirKind getDirectionalType() const override { return type; }

    /// @brief Minimum corner of the scene AABB.
    const Eigen::Vector3f& getAabbMin() const override { return aabbMin; }

    /// @brief Maximum corner of the scene AABB.
    const Eigen::Vector3f& getAabbMax() const override { return aabbMax; }

    /// @brief Flat list of spatial leaves.
    const std::vector<SpatialNode>& getNodes() const override { return nodes; }

private:
    void traverseAndFlatten(uint32_t idx, Eigen::Vector3f pos, Eigen::Vector3f size);
};
