#pragma once

#include <algorithm>
#include <cstdio>

/**
 * @brief Exports a directional heatmap to linear PFM.
 * Each PFM is divided by its own grid maximum so the brightest pixel lands at 1.0;
 * radiance caches are otherwise so dim that any non-auto-ranging viewer shows pure black.
 * The unscaled @p outMin / @p outMax of the grid are returned so a sidecar CSV
 * can record the per-file scale and recover absolute radiance as @c pfm_value * outMax.
 *
 * "Pf\n" as Pf = single channel
 * "WIDTH HEIGHT\n"
 * "SCALE\n" <0 little-endian, >0 big-endian
 * deposit float32 samples, scanlines bottom-to-top.
 */
namespace PFMExport {

    template<int RES>
    bool exportGrayscalePFM(
        const char* filename,
        const float grid[RES][RES],
        float& outMin,
        float& outMax
    ) {
        float vMin = grid[0][0], vMax = grid[0][0];
        for (int y = 0; y < RES; ++y)
            for (int x = 0; x < RES; ++x) {
                vMin = std::min(vMin, grid[y][x]);
                vMax = std::max(vMax, grid[y][x]);
            }
        outMin = vMin;
        outMax = vMax;

        // float scale = (vMax > 0.0f) ? (1.0f / vMax) : 1.0f;

        FILE* result = std::fopen(filename, "wb");
        if (!result) return false;

        if (std::fprintf(result, "Pf\n%d %d\n-1.0\n", RES, RES) < 0) {
            std::fclose(result);
            return false;
        }

        // float row[RES];
        for (int y = 0; y < RES; ++y) {
            // for (int x = 0; x < RES; ++x) row[x] = grid[y][x] * scale;
            if (std::fwrite(grid[y], sizeof(float), RES, result) != (size_t)RES) {
                std::fclose(result);
                return false;
            }
        }

        std::fclose(result);
        return true;
    }
}
