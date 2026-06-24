#pragma once

#include "heatGrid.h"

#include "../../globals/palette.h"
#include "../directional/dirKind.h"

#include <cstdint>
#include <vector>

/**
 * @brief Rasterise a set of directional leaves into a normalised PDF on @p grid.
 *
 * The output integrates to 1 over [0,1]^2 (sum * pixelArea = 1). The leaf bounds
 * are interpreted in @p grid 's own coordinate space, so callers convert leaves
 * to the target parameter space before calling.
 */
void rasterizeLeavesPDF(const std::vector<DirLeaf>& leaves, HeatGrid& grid);

/**
 * @brief Paint each leaf's raw radiance into @p grid (no normalisation).
 *
 * Used by the quadtree-style views that show per-leaf radiance/cost directly.
 */
void paintLeaves(const std::vector<DirLeaf>& leaves, HeatGrid& grid, float floor);

/**
 * @brief Build RGBA pixels from a value grid using a log-scale colormap.
 * @p pixels is resized to grid.w * grid.h * 4.
 */
void buildHeatmapPixels(
    const HeatGrid& grid,
    const std::vector<Color3f>& colormap,
    std::vector<uint8_t>& pixels,
    float valueMin, float valueMax
);

/**
 * @brief Build a 1-pixel-wide vertical colorbar strip ( @p height pixels tall).
 */
void buildColorbarPixels(const std::vector<Color3f>& colormap, uint8_t* pixels, int height);
