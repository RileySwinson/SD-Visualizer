#pragma once

#include "../../globals/bounds.h"

struct BinInfo {
    float radiance  = 0;
    Bounds2f bounds = { 0, 0, 0, 0 };
    float area      = 0;
    float pct       = 0;
    int slotIndex   = -1;
    bool valid      = false;
};
