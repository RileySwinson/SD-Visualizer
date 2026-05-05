#pragma once

#include "dirKind.h"
#include "../../globals/config.h"
#include "../blobReader.h"

#include <vector>
#include <cstddef>

/**
 * @brief Abstract interface for the angular directional distribution stored at a single spatial leaf of an SD-tree.
 *
 * Visualization code (e.g. heatmaps, KL maps) operates against this interface so it stays agnostic of which directional encoding is in use.
 */
class IDirectional {
public:
    virtual ~IDirectional() = default; 

    virtual DirKind kind() const = 0;

    /// @brief Mean radiance over the directional domain.
    virtual float  getMean() const = 0;

    /// @brief Total number of samples accumulated into this distribution.
    virtual size_t getNumSamples() const = 0;

    /// @brief Number of distributions stored. e.g. cost has @c 1 ; 0 for radaince and 1 for cost.
    virtual int getNumDistributions() const = 0;

    /**
     * @brief Evaluate the PDF at directional coordinate @p (u,v) .
     * @param u Horizontal coordinate in [0,1].
     * @param v Vertical coordinate in [0,1].
     * @param distributionIndex Index of distribution to evaluate; @c -1 to use a default.
     * @return PDF value at @p (u,v) .
     */
    virtual float evalPDF(float u, float v, int distributionIndex = -1) const = 0;

    /**
     * @brief Append the leaves of this distribution to @p out .
     * @param out Vector to appended to.
     * @param distributionIndex Index of distribution to evaluate; @c -1 to use a default.
     *
     * Used by visualizations that need a flat list of `(radiance, bounds)` leaves.
     */
    virtual void  collectLeaves(std::vector<DirLeaf>& out, int distributionIndex = -1) const = 0;

    /**
     * @brief Rasterize the distribution into a fixed-resolution grid.
     * @param grid Optional override of grid of size @c HMAP_RES x @c HMAP_RES
     * @param distributionIndex Index of distribution to  rasterize.
     */
    virtual void  fillGrid(float grid[HMAP_RES][HMAP_RES], int distributionIndex = -1) const = 0;

    bool isBTC() const { return kind() == DirKind::BTC; }

    /**
     * @brief Read the directional distribution from the current position in the dump.
     * @param b BlobReader positioned at the start of a payload from the dump.
     * @return @c true on success, @c false otherwise.
     */
    virtual bool read(BlobReader& b) = 0;
};