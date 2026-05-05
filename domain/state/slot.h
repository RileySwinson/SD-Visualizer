#pragma once

#include "../directional/dirKind.h"

#include <Eigen/Dense>

#include <string>
#include <vector>

// a heatmap slot to render
struct Slot {
    std::string title;
    std::string barLabel;
    std::string typeLabel;
    float vMin = 0, vMax = 1;
    bool valid = false;
    bool useKL = false;

    DirKind dirType = QuadTree;
    std::vector<DirLeaf> leaves;
    float totalRad = 0;
    Eigen::Vector3f nodePos{ 0, 0, 0 };

    int datasetIndex = -1;
    int nodeIndex = -1;
    float integral = 0;
    bool isIntegralValid = true;
    int numTilings = 0;
};
