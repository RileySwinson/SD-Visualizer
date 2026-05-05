#pragma once

#include "../../globals/config.h"
#include "../../globals/palette.h"
#include "../directional/dirKind.h"

#include <cstdint>
#include <vector>

/**
 * @brief Rasterise a set of directional leaves into a normalised PDF on the HMAP_RES grid.
 *
 * The output integrates to 1 over [0,1]^2 (sum * pixelArea = 1).
 *
 * @param leaves
 * @param grid
 */
void rasterizeLeavesPDF(const std::vector<DirLeaf>& leaves, float grid[HMAP_RES][HMAP_RES]);

/**
 * @brief Build RGBA pixels from a value grid using a log-scale colormap.
 *
 * @param grid
 * @param colormap
 * @param pixels
 * @param valueMin
 * @param valueMax
 */
void buildHeatmapPixels(
    const float grid[HMAP_RES][HMAP_RES],
    const std::vector<Color3f>& colormap,
    uint8_t pixels[HMAP_RES * HMAP_RES * 4],
    float valueMin, float valueMax
);

/**
 * @brief Build a 1-pixel-wide vertical colorbar strip ( @p height pixels tall).
 *
 * @param colormap
 * @param pixels
 * @param height
 */
void buildColorbarPixels(const std::vector<Color3f>& colormap, uint8_t* pixels, int height);
