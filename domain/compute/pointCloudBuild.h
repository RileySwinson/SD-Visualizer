#pragma once

#include "../spatial/sdtreeSpatial.h"
#include "structureCompare.h"

#include <Eigen/Dense>

#include <vector>

struct PointVertex {
    float x, y, z;
    float r, g, b, a;
};

struct PointCloudResult {
    std::vector<PointVertex> vertices;
    Eigen::Vector3f          sceneMin;
    Eigen::Vector3f          sceneMax;
};

/**
 * @brief Build the point-cloud vertex buffer for the visible leaves.
 *
 * Each leaf becomes a colored point at its centroid. Color is mapped through
 * the inferno colormap on a log-scale of mean radiance.
 *
 * If @p divStatus is non-null, leaves are coloured by structural diff status:
 * orange for @c DIV_REFINED , red for @c DIV_DIVERGENT , normal colormap for
 * @c DIV_UNCHANGED .
 *
 * @param dataset
 * @param visibleIndices
 * @param divStatus
 * @return PointCloudResult
 */
PointCloudResult buildPointCloud(
    const SDTree& dataset,
    const std::vector<int>& visibleIndices,
    const std::vector<DivStatus>* divStatus
);
