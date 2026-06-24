#pragma once

#include "../directional/parameterSpace.h"

#include <Eigen/Dense>

// Lower bound of the directional heatmap colour scale.
//   FromZero   - anchor at radiance 0 (absolute, comparable across heatmaps)
//   AutoRange  - stretch to the per-node [min, max]
enum class HeatmapScale { FromZero, AutoRange };

inline const char* heatmapScaleName(HeatmapScale s) {
    switch (s) {
        case HeatmapScale::FromZero:  return "From 0";
        case HeatmapScale::AutoRange: return "Auto-range (min\xE2\x80\x93max)";
    }
    return "?";
}

// Persistent, mode-independnet display state.
// These flags do not change when the user switches modes.
struct DisplayFlags {
    bool showAllWireframes = false;
    bool showFilterBox     = false;
    bool isFilterActive    = false;

    // Coordinate space all directional heatmaps/KL/exports are rendered in.
    ParameterSpace paramSpace = ParameterSpace::Quadtree;

    // Lower bound of the heatmap colour scale.
    HeatmapScale heatmapScale = HeatmapScale::FromZero;

    Eigen::Vector3f filterMin{ 0, 0, 0 };
    Eigen::Vector3f filterMax{ 1, 1, 1 };
};
