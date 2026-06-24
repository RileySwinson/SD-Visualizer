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

    // Row-major @p data of size @p w * @p h, row 0 = bottom scanline.
    inline bool exportGrayscalePFM(
        const char*  filename,
        const float* data,
        int          w,
        int          h,
        float&       outMin,
        float&       outMax
    ) {
        float vMin = data[0], vMax = data[0];
        for (int i = 0; i < w * h; ++i) {
            vMin = std::min(vMin, data[i]);
            vMax = std::max(vMax, data[i]);
        }
        outMin = vMin;
        outMax = vMax;

        FILE* result = std::fopen(filename, "wb");
        if (!result) return false;

        if (std::fprintf(result, "Pf\n%d %d\n-1.0\n", w, h) < 0) {
            std::fclose(result);
            return false;
        }

        for (int y = 0; y < h; ++y) {
            if (std::fwrite(data + (size_t)y * w, sizeof(float), w, result) != (size_t)w) {
                std::fclose(result);
                return false;
            }
        }

        std::fclose(result);
        return true;
    }
}
