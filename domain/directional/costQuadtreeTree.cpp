#include "costQuadtreeTree.h"

#include <algorithm>

bool CostQuadtreeTree::read(BlobReader& b) {
    uint64_t nn;
    float    ns;
    int      md;
    float    aux;

    b >> mMean >> ns >> nn >> md >> aux;

    float tf;
    int   ti;
    b >> tf >> ti;

    for (int i = 0; i < 3; ++i) {
        b >> tf;
    }

    if (!b.isValid()) return false;

    mNumSamples = (size_t)ns;
    mNodes.resize(nn);

    for (size_t i = 0; i < mNodes.size(); ++i) {
        auto& n = mNodes[i];

        for (int j = 0; j < 4; ++j) {
            for (int k = 0; k < NUM_CHANNELS; ++k) {
                b >> n.mData[k][j];
            }
        
            b >> n.mCosts[j];
            b >> n.mChildren[j];
        }
    }
    return true;
}

float CostQuadtreeTree::evalRadiance(float px, float py, int nodeIndex) const {
    if (nodeIndex < 0 || nodeIndex >= (int)mNodes.size()) return 0;
    auto& n = mNodes[nodeIndex];

    int quadrant;
    if (px < 0.5f) {
        px *= 2;
        if (py < 0.5f) { 
            py *= 2; 
            quadrant = 0; 
        }
        else { 
            py = (py - 0.5f) * 2; 
            quadrant = 1; 
        }
    } else {
        px = (px - 0.5f) * 2;
        if (py < 0.5f) { 
            py *= 2; 
            quadrant = 2;
        }
        else { 
            py = (py - 0.5f) * 2; 
            quadrant = 3; 
        }
    }

    if (n.isLeaf(quadrant)) return n.mData[0][quadrant];
    return 4.0f * evalRadiance(px, py, n.mChildren[quadrant]);
}

float CostQuadtreeTree::evalCost(float px, float py, int nodeIndex) const {
    if (nodeIndex < 0 || nodeIndex >= (int)mNodes.size()) return 0;
    auto& n = mNodes[nodeIndex];

    int quadrant;
    if (px < 0.5f) {
        px *= 2;
        if (py < 0.5f) { 
            py *= 2; 
            quadrant = 0; 
        }
        else { 
            py = (py - 0.5f) * 2; 
            quadrant = 1; 
        }
    } else {
        px = (px - 0.5f) * 2;
        if (py < 0.5f) { 
            py *= 2; 
            quadrant = 2; 
        }
        else { 
            py = (py - 0.5f) * 2; 
            quadrant = 3; 
        }
    }

    if (n.isLeaf(quadrant)) return n.mCosts[quadrant];
    return evalCost(px, py, n.mChildren[quadrant]);
}

float CostQuadtreeTree::evalPDF(float u, float v, int tilingIndex) const {
    if (mNodes.empty()) return 0;
    
    if (tilingIndex == VARIANT_COSTS) {
        return evalCost(u, v, 0);
    }

    if (mNumSamples == 0 || mMean <= 0) return 0;
    float rawEval = evalRadiance(u, v, 0);
    float factor  = 1.0f / ((float)M_PI * mNumSamples);
    return (factor * rawEval) / mMean;
}

void CostQuadtreeTree::collectLeavesRadiance(std::vector<DirLeaf>& out, int nodeIndex, Bounds2f bounds) const {
    if (nodeIndex >= (int)mNodes.size()) return;

    auto& n    = mNodes[nodeIndex];
    float midX = bounds.midA();
    float midY = bounds.midB();

    for (int i = 0; i < 4; ++i) {
        bool isRight = (i >= 2);
        bool isTop   = (i % 2);

        float childMinX = isRight ? midX : bounds.minA;
        float childMinY = isTop   ? midY : bounds.minB;
        float childMaxX = isRight ? bounds.maxA : midX;
        float childMaxY = isTop   ? bounds.maxB : midY;

        Bounds2f newBounds = Bounds2f{ childMinX, childMinY, childMaxX, childMaxY };

        if (n.isLeaf(i)) out.push_back({ n.mData[0][i], newBounds });
        else collectLeavesRadiance(out, n.mChildren[i], newBounds);
    }
}

void CostQuadtreeTree::collectLeavesCosts(std::vector<DirLeaf>& out, int nodeIndex, Bounds2f bounds) const {
    if (nodeIndex >= (int)mNodes.size()) return;

    auto& n    = mNodes[nodeIndex];
    float midX = bounds.midA();
    float midY = bounds.midB();

    for (int i = 0; i < 4; ++i) {
        bool isRight = (i >= 2);
        bool isTop   = (i % 2);

        float childMinX = isRight ? midX : bounds.minA;
        float childMinY = isTop   ? midY : bounds.minB;
        float childMaxX = isRight ? bounds.maxA : midX;
        float childMaxY = isTop   ? bounds.maxB : midY;

        Bounds2f newBounds = Bounds2f{ childMinX, childMinY, childMaxX, childMaxY };

        // DirLeaf::radiance carries the cost value when in costs view.
        if (n.isLeaf(i)) out.push_back({ n.mCosts[i], newBounds });
        else collectLeavesCosts(out, n.mChildren[i], newBounds);
    }
}

void CostQuadtreeTree::collectLeaves(std::vector<DirLeaf>& out, int tilingIndex) const {
    if (tilingIndex == VARIANT_COSTS) collectLeavesCosts(out, 0, Bounds2f::unit());
    else collectLeavesRadiance(out, 0, Bounds2f::unit());
}

void CostQuadtreeTree::fillGrid(float grid[HMAP_RES][HMAP_RES], int tilingIndex) const {
    for (int y = 0; y < HMAP_RES; ++y) {
        for (int x = 0; x < HMAP_RES; ++x) {
            grid[y][x] = PDF_FLOOR;
        }
    }

    std::vector<DirLeaf> leaves;
    if (tilingIndex == VARIANT_COSTS) collectLeavesCosts(leaves, 0, Bounds2f::unit());
    else collectLeavesRadiance(leaves, 0, Bounds2f::unit());

    for (auto& leaf : leaves) {
        Bounds2i px = Bounds2i::fromBounds2f(leaf.bounds, HMAP_RES);
        for (int y = px.minB; y < px.maxB; ++y) {
            for (int x = px.minA; x < px.maxA; ++x) {
                grid[y][x] = std::max(leaf.radiance, PDF_FLOOR);
            }
        }
    }
}
