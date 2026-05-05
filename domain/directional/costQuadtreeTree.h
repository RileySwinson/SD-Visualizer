#pragma once

#include "iDirectional.h"

#include <array>
#include <cstdint>
#include <vector>

// One node of a cost-augmented directional quadtree. Each of the 4 quadrants
// stores both an accumulated radiance (`sum`) and an accumulated cost
// (`sumCosts`), plus the optional child-node index.
struct CostQTNode {
    std::array<std::array<float, 4>, NUM_CHANNELS> mData;     // radiance per quadrant
    std::array<float, 4>                           mCosts;    // cost per quadrant
    std::array<uint16_t, 4>                        mChildren;

    bool isLeaf(int i) const { return mChildren[i] == 0; }
};

// Cost-augmented quadtree directional cache.
//
// Identical layout to QuadtreeTree, but each leaf has both a radiance and a
// cost. Two view variants are exposed via `tilingIndex`:
//   tilingIndex <= 0  -> radiance (default; behaves identically to QuadTree)
//   tilingIndex == 1  -> costs    (paint raw cost per leaf)
//
// The directional panel renders these as labeled "Radiance" / "Costs" buttons
// instead of the BTC-style "Avg / 1 / 2 / ..." numbered tilings.
class CostQuadtreeTree : public IDirectional {
public:
    DirKind kind() const override { return DirKind::CostQuadTree; }

    float  getMean()        const override { return mMean; }
    size_t getNumSamples()  const override { return mNumSamples; }
    int    getNumDistributions()  const override { return 0; }

    float evalPDF(float u, float v, int tilingIndex = -1) const override;
    void  collectLeaves(std::vector<DirLeaf>& out, int tilingIndex = -1) const override;
    void  fillGrid(float grid[HMAP_RES][HMAP_RES], int tilingIndex = -1) const override;

    bool read(BlobReader& b) override;

    // Variant index used to mean "show costs" (vs. radiance for any other value).
    static constexpr int VARIANT_COSTS = 1;

private:
    std::vector<CostQTNode> mNodes;
    float                   mMean       = 0;
    std::size_t             mNumSamples = 0;

    float evalRadiance(float px, float py, int nodeIndex) const;
    float evalCost(float px, float py, int nodeIndex) const;

    void collectLeavesRadiance(std::vector<DirLeaf>& out, int nodeIndex, Bounds2f bounds) const;
    void collectLeavesCosts(std::vector<DirLeaf>& out, int nodeIndex, Bounds2f bounds) const;
};
