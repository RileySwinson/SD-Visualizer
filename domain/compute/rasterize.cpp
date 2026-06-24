#include "rasterize.h"

#include "../../globals/bounds.h"
#include "../../globals/config.h"

#include <algorithm>
#include <cmath>

void rasterizeLeavesPDF(const std::vector<DirLeaf>& leaves, HeatGrid& grid) {
    for (auto& v : grid.data) v = PDF_FLOOR;

    // Total radiance for normalisation.
    float totalRad = 0;
    for (auto& leaf : leaves) totalRad += std::max(leaf.radiance, PDF_FLOOR);
    if (totalRad < PDF_FLOOR) totalRad = PDF_FLOOR;

    // Paint each leaf as its area-divided probability mass.
    for (auto& leaf : leaves) {
        Bounds2i px   = Bounds2i::fromBounds2f(leaf.bounds, grid.w, grid.h);
        float    area = leaf.bounds.area();

        if (area > 0) {
            float density = (std::max(leaf.radiance, PDF_FLOOR) / totalRad) / area;
            for (int y = px.minB; y < px.maxB; ++y)
                for (int x = px.minA; x < px.maxA; ++x)
                    grid.at(x, y) = density;
        }
    }

    // Normalise so that sum * pixelArea == 1.
    float sum       = 0;
    float pixelArea = 1.0f / (grid.w * grid.h);
    for (auto& v : grid.data) sum += v;
    sum *= pixelArea;

    if (sum > 0)
        for (auto& v : grid.data) v /= sum;
}

void paintLeaves(const std::vector<DirLeaf>& leaves, HeatGrid& grid, float floor) {
    for (auto& v : grid.data) v = floor;

    for (auto& leaf : leaves) {
        Bounds2i px = Bounds2i::fromBounds2f(leaf.bounds, grid.w, grid.h);
        for (int y = px.minB; y < px.maxB; ++y)
            for (int x = px.minA; x < px.maxA; ++x)
                grid.at(x, y) = std::max(leaf.radiance, floor);
    }
}

void buildHeatmapPixels(
    const HeatGrid& grid,
    const std::vector<Color3f>& colormap,
    std::vector<uint8_t>& pixels,
    float valueMin, float valueMax
) {
    pixels.resize((std::size_t)grid.w * grid.h * 4);

    float logMin  = std::log(std::max(valueMin, LOG_PDF_FLOOR));
    float logMax  = std::log(std::max(valueMax, logMin + 1.f));
    float logSpan = logMax - logMin;
    if (logSpan < 1e-6f) logSpan = 1;

    for (int y = 0; y < grid.h; ++y) {
        for (int x = 0; x < grid.w; ++x) {
            float normLog = (std::log(std::max(grid.at(x, y), valueMin)) - logMin) / logSpan;
            normLog = std::max(0.f, std::min(1.f, normLog));

            Color3f c = sampleColormap(colormap, normLog);

            std::size_t o = ((std::size_t)y * grid.w + x) * 4;
            pixels[o]     = (uint8_t)(c.r * 255);
            pixels[o + 1] = (uint8_t)(c.g * 255);
            pixels[o + 2] = (uint8_t)(c.b * 255);
            pixels[o + 3] = 255;
        }
    }
}

void buildColorbarPixels(
    const std::vector<Color3f>& colormap,
    uint8_t* pixels, int height
) {
    for (int y = 0; y < height; ++y) {
        float   t = (float)y / (height - 1);
        Color3f c = sampleColormap(colormap, t);

        int o = y * 4;
        pixels[o]     = (uint8_t)(c.r * 255);
        pixels[o + 1] = (uint8_t)(c.g * 255);
        pixels[o + 2] = (uint8_t)(c.b * 255);
        pixels[o + 3] = 255;
    }
}
