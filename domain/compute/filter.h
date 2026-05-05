#pragma once

#include "../spatial/sdtreeSpatial.h"

#include <Eigen/Geometry>

#include <vector>

/**
 * @brief Gets indices of all spatial leaves whose AABB intersects `filterBox`.
 * 
 * @param dataset 
 * @param filterBox 
 * @return std::vector<int> 
 */
std::vector<int> computeFilteredIndices(const SDTree& dataset, const Eigen::AlignedBox3f& filterBox);

/**
 * @brief Sum of mean radiance over leaves intersecting `filterBox`. If `applyFilter`is false, sums all leaves (and filterBox is unused).
 * 
 * @param dataset 
 * @param filterBox 
 * @param applyFilter 
 * @return float 
 */
float filteredTotalRadiance(const SDTree& dataset, const Eigen::AlignedBox3f& filterBox, bool applyFilter);
