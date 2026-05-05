#pragma once

#include <cstdint>
#include <cstdio>
#include <vector>
#include <algorithm>
#include <zlib.h>

// I now know the PNGG format in the bare minimum amount of detail i needed to make this
// honestly a really educational exercise
namespace PNGExport {
    static void writeChunk(FILE* f, const char type[4], const uint8_t* data, uint32_t len) {
        
        uint8_t bigEdian[4] = {
            (uint8_t)((len >> 24) & 0xFF), (uint8_t)((len >> 16) & 0xFF),
            (uint8_t)((len >> 8) & 0xFF),  (uint8_t)(len & 0xFF)
        };
        
        fwrite(bigEdian, 1, 4, f);
        fwrite(type, 1, 4, f);
        
        if (data && len > 0) fwrite(data, 1, len, f);

        // cec is Cyclic Redundancy Check. 
        // CRC32 over type + data
        uint32_t crc = crc32(0L, Z_NULL, 0);
        crc = crc32(crc, (const Bytef*)type, 4);
        if (data && len > 0) crc = crc32(crc, data, len);

        uint8_t crcBE[4] = {
            (uint8_t)((crc >> 24) & 0xFF), (uint8_t)((crc >> 16) & 0xFF),
            (uint8_t)((crc >> 8) & 0xFF),  (uint8_t)(crc & 0xFF)
        };
        fwrite(crcBE, 1, 4, f);
    }

    // Export a float grid as a linear grayscale PNG.
    // Grid row 0 = bottom of displayed heatmap, so rows are flipped
    // to match the on-screen orientation (PNG row 0 = top).
    template<int RES>
    bool exportGrayscalePNG(const char* filename, const float grid[RES][RES]) {
        float vMin = grid[0][0], vMax = grid[0][0];
        for (int y = 0; y < RES; ++y)
            for (int x = 0; x < RES; ++x) {
                vMin = std::min(vMin, grid[y][x]);
                vMax = std::max(vMax, grid[y][x]);
            }

        float range = vMax - vMin;
        if (range < 1e-12f) range = 1.0f;

        // Build raw scanlines. each row is [filter_byte = 0] [RES grayscale bytes]
        // Then flip Y so PNG top row = grid's last row
        size_t rowBytes = 1 + RES;
        std::vector<uint8_t> raw(RES * rowBytes);

        for (int y = 0; y < RES; ++y) {
            int srcRow = RES - 1 - y; 
            raw[y * rowBytes] = 0; // no PNG filter
            for (int x = 0; x < RES; ++x) {
                float t = (grid[srcRow][x] - vMin) / range;
                t = std::max(0.0f, std::min(1.0f, t));
                raw[y * rowBytes + 1 + x] = (uint8_t)(t * 255.0f + 0.5f);
            }
        }

        // Compress with zlib (PNG requires deflate-compressed image data)
        uLongf compressLen = compressBound(raw.size());
        std::vector<uint8_t> compressed(compressLen);
        if (compress2(compressed.data(), &compressLen, raw.data(), raw.size(), Z_DEFAULT_COMPRESSION) != Z_OK) {
            return false;
        }

        compressed.resize(compressLen);

        FILE* f = fopen(filename, "wb");
        if (!f) return false;

        // PNG signature (magic bytes identifying the file as PNG)
        const uint8_t sig[8] = {137, 80, 78, 71, 13, 10, 26, 10};
        fwrite(sig, 1, 8, f);

        // IHDR: width, height (big-endian u32 each), bit depth, color type, compression, filter, interlace
        // stole this from the internet ngl
        uint8_t ihdr[13];
        ihdr[0]  = (RES >> 24) & 0xFF; ihdr[1]  = (RES >> 16) & 0xFF;
        ihdr[2]  = (RES >> 8)  & 0xFF; ihdr[3]  =  RES        & 0xFF;
        ihdr[4]  = (RES >> 24) & 0xFF; ihdr[5]  = (RES >> 16) & 0xFF;
        ihdr[6]  = (RES >> 8)  & 0xFF; ihdr[7]  =  RES        & 0xFF;
        ihdr[8]  = 8; // 8 bits per channel
        ihdr[9]  = 0; // color type 0 = grayscale
        ihdr[10] = 0; // compression method (only 0 = deflate is defined)
        ihdr[11] = 0; // filter method (only 0 = adaptive is defined)
        ihdr[12] = 0; // no interlacing
        writeChunk(f, "IHDR", ihdr, 13);

        // IDAT: the compressed image data
        writeChunk(f, "IDAT", compressed.data(), (uint32_t)compressed.size());

        // IEND: empty chunk marking end of file
        writeChunk(f, "IEND", nullptr, 0);

        fclose(f);
        return true;
    }

}