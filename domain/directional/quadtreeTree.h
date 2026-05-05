#pragma once

#include "iDirectional.h"

#include <array>
#include <cstdint>
#include <vector>

/**
 * @brief Stores one node of a directional quadtree. Each quadrand is either a leaf with radiance of an index of a parent cell.
 * Adapted from Muller's code. 
 */
 struct QTNode {
    std::array<std::array<float, 4>, NUM_CHANNELS> mData;
    std::array<uint16_t, 4> mChildren;

    bool  isLeaf(int i) const { return mChildren[i] == 0; }
    float eval(float px, float py, const std::vector<QTNode>& nodes) const;
};

/// Quadtree directional cache adapted from muller
class QuadtreeTree : public IDirectional {
public:
    DirKind kind() const override { return DirKind::QuadTree; }

    float  getMean()                const override { return mMean; }
    size_t getNumSamples()          const override { return mNumSamples; }
    int    getNumDistributions()    const override { return 0; }

    float evalPDF(float u, float v, int tilingIndex = -1) const override;
    void  collectLeaves(std::vector<DirLeaf>& out, int tilingIndex = -1) const override;
    void  fillGrid(float grid[HMAP_RES][HMAP_RES], int tilingIndex = -1) const override;

    bool read(BlobReader& b) override;

private:
    std::vector<QTNode> mNodes;
    float               mMean       = 0;
    std::size_t         mNumSamples = 0;

    void collectLeavesQT(std::vector<DirLeaf>& out, int nodeIndex, Bounds2f bounds) const;
};
