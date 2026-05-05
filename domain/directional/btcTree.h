#pragma once

#include "iDirectional.h"

#include <cstdint>
#include <vector>

/**
 * @brief One tile within a BTC tiling. Either a leaf, or an internal split with two children.
 */
struct VisTile {
    float    accRad         = 0; 
    uint32_t sampleCount    = 0;
    uint8_t  isInternal     = 0;
    float    power[2]       = {};
    uint32_t children[2]    = {};
    uint8_t  splitDirection = 0;
    float    covX, covY           = 0;
    float    meanX,meanY          = 0;
    float    m2             = 0;
    float    adSum          = 0;

    bool isLeaf() const { return isInternal == 0; }
};

/**
 * @brief One BTC tiling is a rectangular subdivision of UV space (may extend beyond [0,1] due to padding).
 */
struct VisTiling {
    Bounds2f domain = Bounds2f{ 0.0f, 0.0f, 1.0f, 1.0f };
    float totalPower = 0;
    uint32_t baseDims[2] = { 1, 1 };
    std::vector<VisTile> tiles;

    bool read(BlobReader& b);
};

/**
 * @brief BTC (Binary Tile Coding) directional cache.
 */
class BtcTree : public IDirectional {
public:
    DirKind kind() const override { return DirKind::BTC; }

    float  getMean()        const override { return mMean; }
    size_t getNumSamples()  const override { return mNumSamples; }
    int    getNumDistributions()  const override { return (int)mTilings.size(); }

    float evalPDF(float u, float v, int tilingIndex = -1) const override;
    void  collectLeaves(std::vector<DirLeaf>& out, int tilingIndex = -1) const override;
    void  fillGrid(float grid[HMAP_RES][HMAP_RES], int tilingIndex = -1) const override;

    bool read(BlobReader& b) override;

private:
    std::vector<VisTiling> mTilings;
    float                  mMean       = 0;
    float                  mLeafSum    = 0;
    std::size_t            mNumSamples = 0;

    void traverseTileLeaves(
        const VisTiling& tiling, int tileIndex,
        Bounds2f local, float xRange, float yRange,
        float xOffset, float yOffset,
        std::vector<DirLeaf>& out
    ) const;
};
