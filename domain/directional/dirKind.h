#pragma once

#include "../../globals/bounds.h"

enum DirKind { QuadTree, BTC, CostQuadTree, Count };

inline const char* dirKindToName(DirKind kind) {
    switch (kind) {
        case BTC: return "BTC";
        case CostQuadTree: return "Quadtree+Costs";
        case QuadTree: return "Quadtree";
        default: return "Quadtree";
    }
}
struct DirLeaf {
    float    radiance;
    Bounds2f bounds;
};
