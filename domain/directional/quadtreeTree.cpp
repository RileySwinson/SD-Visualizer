#include "quadtreeTree.h"

#include <algorithm>

// Quadrant layout when subdividing [0,1]^2:
//   c=0: x<0.5, y<0.5  (bottom-left)
//   c=1: x<0.5, y>=0.5 (top-left)
//   c=2: x>=0.5, y<0.5 (bottom-right)
//   c=3: x>=0.5, y>=0.5 (top-right)
// Coords are remapped to [0,1] and the result is multiplied by 4 to compensate
// for the area reduction at each level.
float QTNode::eval(float px, float py, const std::vector<QTNode>& nodes) const {
    int quadrant;

    if (px < 0.5f) {
        px *= 2;
        if (py < 0.5f) { py *= 2;                quadrant = 0; }
        else           { py = (py - 0.5f) * 2;    quadrant = 1; }
    } else {
        px = (px - 0.5f) * 2;
        if (py < 0.5f) { py *= 2;                quadrant = 2; }
        else           { py = (py - 0.5f) * 2;    quadrant = 3; }
    }

    if (isLeaf(quadrant)) return mData[0][quadrant];
    return 4.0f * nodes[mChildren[quadrant]].eval(px, py, nodes);
}

bool QuadtreeTree::read(BlobReader& b) {
    uint64_t nn;
    float    ns;
    int      md;
    float    aux;

    b >> mMean >> ns >> nn >> md >> aux;

    float tf;
    int   ti;
    b >> tf >> ti;

    for (int i = 0; i < 3; ++i) b >> tf;

    if (!b.isValid()) return false;

    mNumSamples = (size_t)ns;
    mNodes.resize(nn);

    for (size_t i = 0; i < mNodes.size(); ++i) {
        auto& n = mNodes[i];
        for (int j = 0; j < 4; ++j) {
            for (int k = 0; k < NUM_CHANNELS; ++k) {
                b >> n.mData[k][j];
            }
            b >> n.mChildren[j];
        }
    }
    return true;
}

float QuadtreeTree::evalPDF(float u, float v, int /*tilingIndex*/) const {
    if (mNodes.empty() || mNumSamples == 0 || mMean <= 0) return 0;

    // Muller's normalisation: factor / (pi * numSamples)
    float rawEval = mNodes[0].eval(u, v, mNodes);
    float factor  = 1.0f / ((float)M_PI * mNumSamples);
    return (factor * rawEval) / mMean;
}

void QuadtreeTree::collectLeavesQT(std::vector<DirLeaf>& out, int nodeIndex, Bounds2f bounds) const {
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
        else             collectLeavesQT(out, n.mChildren[i], newBounds);
    }
}

void QuadtreeTree::collectLeaves(std::vector<DirLeaf>& out, int /*distributionIndex*/) const {
    collectLeavesQT(out, 0, Bounds2f::unit());
}

void QuadtreeTree::fillGrid(float grid[HMAP_RES][HMAP_RES], int /*distributionIndex*/) const {
    for (int y = 0; y < HMAP_RES; ++y)
        for (int x = 0; x < HMAP_RES; ++x)
            grid[y][x] = PDF_FLOOR;

    std::vector<DirLeaf> leaves;
    collectLeavesQT(leaves, 0, Bounds2f::unit());

    for (auto& leaf : leaves) {
        Bounds2i px = Bounds2i::fromBounds2f(leaf.bounds, HMAP_RES);
        for (int y = px.minB; y < px.maxB; ++y)
            for (int x = px.minA; x < px.maxA; ++x)
                grid[y][x] = std::max(leaf.radiance, PDF_FLOOR);
    }
}
