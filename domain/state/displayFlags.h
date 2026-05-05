#pragma once

#include <Eigen/Dense>

// Persistent, mode-independnet display state.
// These flags do not change when the user switches modes.
struct DisplayFlags {
    bool showAllWireframes = false;
    bool showFilterBox     = false;
    bool isFilterActive    = false;

    Eigen::Vector3f filterMin{ 0, 0, 0 };
    Eigen::Vector3f filterMax{ 1, 1, 1 };
};
