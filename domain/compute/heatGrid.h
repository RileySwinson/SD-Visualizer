#pragma once

#include <cstddef>
#include <vector>

// A variable-size heatmap grid. Replaces the fixed HMAP_RES x HMAP_RES arrays so
// non-square parameter spaces (e.g. the 2:1 spherical lat-long) can be rendered.
// Row 0 is the bottom row (matches the existing PFM / texture orientation).
struct HeatGrid {
    int                w = 0;
    int                h = 0;
    std::vector<float> data;

    void resize(int width, int height, float fill = 0.0f) {
        w = width;
        h = height;
        data.assign((std::size_t)width * height, fill);
    }

    float&      at(int x, int y)       { return data[(std::size_t)y * w + x]; }
    const float& at(int x, int y) const { return data[(std::size_t)y * w + x]; }
};
