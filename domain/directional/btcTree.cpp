#include "btcTree.h"

#include "../projection.h"

#include <algorithm>
#include <cmath>

bool VisTiling::read(BlobReader& b) {
    b >> domain.minA >> domain.maxA >> domain.minB >> domain.maxB
      >> totalPower >> baseDims[0] >> baseDims[1];

    uint32_t tileCount;
    b >> tileCount;
    tiles.resize(tileCount);

    for (uint32_t j = 0; j < tileCount; ++j) {
        auto& t = tiles[j];
        b >> t.accRad >> t.sampleCount >> t.isInternal;
        if (t.isInternal == 1)
            b >> t.power[0] >> t.power[1] >> t.children[0] >> t.children[1] >> t.splitDirection;
        else
            b >> t.covX >> t.covY >> t.meanX >> t.meanY >> t.m2 >> t.adSum;
    }

    return b.isValid();
}

bool BtcTree::read(BlobReader& b) {
    uint32_t numberOfTiles;
    b >> numberOfTiles;

    float leafSum;
    b >> leafSum;
    mLeafSum = leafSum;

    mTilings.resize(numberOfTiles);
    mNumSamples = 0;

    for (uint32_t i = 0; i < numberOfTiles; ++i) {
        if (!mTilings[i].read(b)) return false;
    }

    if (!mTilings.empty()) {
        for (auto& t : mTilings[0].tiles) {
            if (t.isLeaf()) mNumSamples += t.sampleCount;
        }
    }

    mMean = leafSum / (4.0f * (float)M_PI);
    return true;
}

float BtcTree::evalPDF(float u, float v, int tilingIndex) const {
    if (mLeafSum <= 0 || mTilings.empty()) return 1.0f / (4.0f * (float)M_PI);

    float accDensity = 0;
    int   start = (tilingIndex >= 0) ? tilingIndex : 0;
    int   end   = (tilingIndex >= 0) ? tilingIndex + 1 : (int)mTilings.size();

    for (int ti = start; ti < end && ti < (int)mTilings.size(); ++ti) {
        auto& tiling = mTilings[ti];
        float xRange = tiling.domain.width();
        float yRange = tiling.domain.height();

        if (xRange <= 0 || yRange <= 0) continue;

        float warpedV     = EquirectToEqualArea::inverse(v);
        float normalizedX = (u - tiling.domain.minA) / xRange;
        float normalizedY = (warpedV - tiling.domain.minB) / yRange;

        if (normalizedX < 0 || normalizedX >= 1 || normalizedY < 0 || normalizedY >= 1) continue;

        // Adapted from BinaryTiling::find_tile.
        int gridX = std::min((int)(normalizedX * tiling.baseDims[0]), (int)tiling.baseDims[0] - 1);
        int gridY = std::min((int)(normalizedY * tiling.baseDims[1]), (int)tiling.baseDims[1] - 1);
        int tileIndex = gridY * tiling.baseDims[0] + gridX;

        float tileWidth  = xRange / tiling.baseDims[0];
        float tileHeight = yRange / tiling.baseDims[1];
        float boundsMinX = tiling.domain.minA + gridX * tileWidth;
        float boundsMaxX = boundsMinX + tileWidth;
        float boundsMinY = tiling.domain.minB + gridY * tileHeight;
        float boundsMaxY = boundsMinY + tileHeight;

        while (tileIndex >= 0 && tileIndex < (int)tiling.tiles.size() && !tiling.tiles[tileIndex].isLeaf()) {
            auto& tile = tiling.tiles[tileIndex];

            if (tiling.tiles[tile.children[0]].accRad == 0 &&
                tiling.tiles[tile.children[1]].accRad == 0) break;

            bool  hSplit = (tile.splitDirection == 0);
            float mid    = hSplit ? (boundsMinX + boundsMaxX) * 0.5f : (boundsMinY + boundsMaxY) * 0.5f;
            bool  firstChild;

            if (hSplit) {
                firstChild = (u < mid);
            } else {
                firstChild = (v < EquirectToEqualArea::forward(mid));
            }

            if (firstChild) {
                if (hSplit) boundsMaxX = mid;
                else        boundsMaxY = mid;
                tileIndex = tile.children[0];
            } else {
                if (hSplit) boundsMinX = mid;
                else        boundsMinY = mid;
                tileIndex = tile.children[1];
            }
        }

        if (tileIndex >= 0 && tileIndex < (int)tiling.tiles.size()) {
            float clampedMinX = std::max(0.f, boundsMinX);
            float clampedMaxX = std::min(1.f, boundsMaxX);
            float clampedMinY = std::max(0.f, boundsMinY);
            float clampedMaxY = std::min(1.f, boundsMaxY);

            if (clampedMinX < clampedMaxX && clampedMinY < clampedMaxY) {
                float area = EquirectToEqualArea::solidAngleArea(clampedMinX, clampedMaxX, clampedMinY, clampedMaxY);
                if (area > 0) accDensity += tiling.tiles[tileIndex].accRad / area;
            }
        }
    }

    return std::max(PDF_FLOOR, accDensity / mLeafSum);
}

void BtcTree::traverseTileLeaves(
    const VisTiling& tiling, int tileIndex,
    Bounds2f local, float xRange, float yRange,
    float xOffset, float yOffset,
    std::vector<DirLeaf>& out
) const {
    if (tileIndex < 0 || tileIndex >= (int)tiling.tiles.size()) return;
    auto& t = tiling.tiles[tileIndex];

    if (t.isLeaf()) {
        // Clamp warped coords to [0,1] (BTC padding may extend beyond).
        Bounds2f warpedBounds = {
            std::clamp(EquirectToEqualArea::tilingLocalToWarped(local.minA, xRange, xOffset), 0.0f, 1.0f),
            std::clamp(EquirectToEqualArea::tilingLocalToWarped(local.minB, yRange, yOffset), 0.0f, 1.0f),
            std::clamp(EquirectToEqualArea::tilingLocalToWarped(local.maxA, xRange, xOffset), 0.0f, 1.0f),
            std::clamp(EquirectToEqualArea::tilingLocalToWarped(local.maxB, yRange, yOffset), 0.0f, 1.0f)
        };

        if (warpedBounds.isEmpty()) return;

        float area    = EquirectToEqualArea::solidAngleArea(warpedBounds);
        float density = (area > 0 && mLeafSum > 0) ? (t.accRad / area) / mLeafSum : PDF_FLOOR;

        // Convert warped v to equirectangular v for display bounds.
        float equirectMinV = EquirectToEqualArea::forward(warpedBounds.minB);
        float equirectMaxV = EquirectToEqualArea::forward(warpedBounds.maxB);

        out.push_back({ density, Bounds2f{ warpedBounds.minA, equirectMinV, warpedBounds.maxA, equirectMaxV } });

    } else if (t.splitDirection == 0) {
        float midX = local.midA();
        traverseTileLeaves(tiling, t.children[0],
                           { local.minA, local.minB, midX, local.maxB },
                           xRange, yRange, xOffset, yOffset, out);
        traverseTileLeaves(tiling, t.children[1],
                           { midX, local.minB, local.maxA, local.maxB },
                           xRange, yRange, xOffset, yOffset, out);
    } else {
        float midY = local.midB();
        traverseTileLeaves(tiling, t.children[0],
                           { local.minA, local.minB, local.maxA, midY },
                           xRange, yRange, xOffset, yOffset, out);
        traverseTileLeaves(tiling, t.children[1],
                           { local.minA, midY, local.maxA, local.maxB },
                           xRange, yRange, xOffset, yOffset, out);
    }
}

void BtcTree::collectLeaves(std::vector<DirLeaf>& out, int tilingIndex) const {
    if (mTilings.empty()) return;

    int start = (tilingIndex >= 0) ? tilingIndex : 0;
    int end   = (tilingIndex >= 0) ? tilingIndex + 1 : 1;

    for (int ti = start; ti < end && ti < (int)mTilings.size(); ++ti) {
        auto& tiling = mTilings[ti];
        float xRange = tiling.domain.width();
        float yRange = tiling.domain.height();
        float cellW  = 1.f / tiling.baseDims[0];
        float cellH  = 1.f / tiling.baseDims[1];

        for (int row = 0; row < (int)tiling.baseDims[1]; ++row) {
            for (int col = 0; col < (int)tiling.baseDims[0]; ++col) {
                int rootTile = row * tiling.baseDims[0] + col;

                Bounds2f localBounds = {
                    col * cellW,
                    row * cellH,
                    (col + 1) * cellW,
                    (row + 1) * cellH
                };

                traverseTileLeaves(tiling, rootTile, localBounds,
                                   xRange, yRange,
                                   tiling.domain.minA, tiling.domain.minB, out);
            }
        }
    }
}

void BtcTree::fillGrid(float grid[HMAP_RES][HMAP_RES], int tilingIndex) const {
    for (int y = 0; y < HMAP_RES; ++y) {
        for (int x = 0; x < HMAP_RES; ++x) {
            float eval = evalPDF(((float)x + 0.5f) / HMAP_RES,
                                 ((float)y + 0.5f) / HMAP_RES,
                                 tilingIndex);
            grid[y][x] = std::max(eval, PDF_FLOOR);
        }
    }
}
