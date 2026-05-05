#pragma once

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

inline constexpr float PDF_FLOOR     = 1e-9f;
inline constexpr float LOG_PDF_FLOOR = 1e-12f;

inline constexpr int NUM_CHANNELS   = 1;   // total channels; can grow to support, e.g., RGB dumps
inline constexpr int HMAP_RES       = 256; // heatmap grid resolution
inline constexpr int MAX_COMP_SLOTS = 8;
