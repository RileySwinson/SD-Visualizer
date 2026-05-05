#pragma once

#include "config.h"

#include <imgui.h>

#include <array>

inline const ImVec4 COL_BG     = {0.110f, 0.106f, 0.133f, 1.0f};
inline const ImVec4 COL_TEXT   = {0.976f, 0.976f, 0.980f, 1.0f};
inline const ImVec4 COL_ORANGE = {1.000f, 0.443f, 0.224f, 1.0f};
inline const ImVec4 COL_PURPLE = {0.565f, 0.349f, 1.000f, 1.0f};
inline const ImVec4 COL_GREEN  = {0.188f, 0.902f, 0.043f, 1.0f};
inline const ImVec4 COL_CYAN   = {0.000f, 0.867f, 1.000f, 1.0f};
inline const ImVec4 COL_RED    = {1.000f, 0.310f, 0.369f, 1.0f};
inline const ImVec4 COL_BLUE   = {0.000f, 0.376f, 0.875f, 1.0f};

// Per-slot color swatches in PARTIAL_COMP mode (Mozilla branding palette).
inline const std::array<float, 4> COMP_SLOT_COLORS[MAX_COMP_SLOTS] = {
    {1.000f, 0.443f, 0.224f, 1.0f}, // orange
    {0.565f, 0.349f, 1.000f, 1.0f}, // purple
    {0.000f, 0.867f, 1.000f, 1.0f}, // cyan
    {0.188f, 0.902f, 0.043f, 1.0f}, // green
    {1.000f, 0.310f, 0.369f, 1.0f}, // red
    {1.000f, 0.820f, 0.000f, 1.0f}, // yellow
    {0.400f, 0.800f, 0.667f, 1.0f}, // teal
    {0.900f, 0.500f, 0.900f, 1.0f}, // pink
};
