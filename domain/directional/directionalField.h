#pragma once

#include "dirKind.h"
#include "parameterSpace.h"

#include <vector>

// A set of directional leaves together with the parameter space they live in.
// toParameterSpace() re-expresses the leaves in another space using the shared
// mappings in parameterSpace.h. Only the v (polar) bounds warp; u is unchanged.
struct DirectionalField {
    std::vector<DirLeaf> leaves;
    ParameterSpace       space = ParameterSpace::Quadtree;

    DirectionalField toParameterSpace(ParameterSpace target) const;
};
