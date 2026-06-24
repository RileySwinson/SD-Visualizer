#pragma once

#include "../globals/bounds.h"
#include "../globals/config.h"

#include <Eigen/Dense>
#include <cmath>
#include <algorithm>

/**
 * @brief Equirectangular UV ↔ equal-area transformations.
 *
 * Equirectangular UV space: \f$u \in [0,1] \to \phi = 2\pi u\f$ and \f$v \in [0,1] \to \theta = \pi v\f$.
 * The equal-area transform stretches @c v to compensate for the sphere's metric so equal areas in warped @c (u,y) space correspond to equal solid angles on the sphere.
 */
namespace EquirectToEqualArea {

    /**
     * @brief Map equirectangular @c v to warped @c y .
     * \f[ y = \frac{1 - \cos(\pi v)}{2} \f]
     * @param vEquirect Equirectangular vertical coordinate in [0,1].
     * @return Warped vertical coordinate in [0,1].
     */
    inline float inverse(float vEquirect) { return (1.0f - cosf(M_PI * vEquirect)) * 0.5f; }

    /**
     * @brief Map warped @c y back to equirectangular @c v .
     * \f[ v = \frac{\arccos(1 - 2y)}{\pi} \f]
     * @param yWarped Warped vertical coordinate in [0,1].
     * @return Equirectangular vertical coordinate in [0,1].
     */
    inline float forward(float yWarped)   { return acosf(1.f - 2.f * yWarped) / M_PI; }

    /**
     * @brief Solid-angle Jacobian factor.
     * \f[ d\Omega = 2\pi^2 \sin(\pi v)\, du\, dv \f]
     * @param vEquirect Equirectangular vertical coordinate.
     * @return Jacobian factor \f$2\pi^2 \sin(\pi v)\f$.
     */
    inline float jacobian(float vEquirect) { return 2.f * M_PI * M_PI * sinf(M_PI * vEquirect); }

    /**
     * @brief Solid angle (in steradians) of a rectangle in equal-area UV space.
     *
     * \f[ \Delta\Omega = \Delta\theta \cdot \Delta\phi \f]
     * where \f$\Delta\theta = (1 - 2 y_{\min}) - (1 - 2 y_{\max})\f$ and \f$\Delta\phi = 2\pi (x_{\max} - x_{\min})\f$.
     *
     * @param xMin Left edge in @c u .
     * @param xMax Right edge in @c u .
     * @param yMin Bottom edge in warped @c y .
     * @param yMax Top edge in warped @c y .
     * @return Solid angle in steradians.
     */
    inline float solidAngleArea(float xMin, float xMax, float yMin, float yMax) {
        float dTheta = (1.f - 2.f * yMin) - (1.f - 2.f * yMax);
        float dPhi   = 2.f * M_PI * (xMax - xMin);
        return dTheta * dPhi;
    }

    /// @brief Solid angle (in steradians) of a @ref Bounds2f in equal-area UV space.
    inline float solidAngleArea(Bounds2f bounds) {
        float dTheta = (1.f - 2.f * bounds.minB) - (1.f - 2.f * bounds.maxB);
        float dPhi   = 2.f * M_PI * (bounds.maxA - bounds.minA);
        return dTheta * dPhi;
    }

    /**
     * @brief Linearly remap a tiling-local coordinate to warped UV space.
     * \f[ \text{warped} = \text{offset} + \text{local} \cdot \text{range} \f]
     * @param local  Local coordinate inside the tiling, typically in [0,1].
     * @param range  Width of the tiling in warped space.
     * @param offset Tiling origin in warped space.
     * @return The warped coordinate.
     */
    inline float tilingLocalToWarped(float local, float range, float offset) {
        return offset + local * range;
    }
}

/**
 * @brief Cylindrical equal-area projection used by BTC, which takes @c (u,v) directly.
 *
 * Maps \f$(u,v) \in [0,1]^2\f$ to a unit-sphere direction with uniform area distortion.
 */
namespace CylindricalEqualArea {

    /**
     * @brief Map @p (u,v) in \f$ [0,1] ^2 \f$ to a unit-length direction.
     * \f[ \phi = 2\pi u, \quad \cos\theta = 1 - 2v \f]
     * \f[ \mathbf{d} = (\sin\theta \cos\phi,\ \cos\theta,\ \sin\theta \sin\phi) \f]
     * @param u Horizontal coordinate in [0,1].
     * @param v Vertical coordinate in [0,1].
     * @return Unit-length direction vector.
     */
    inline Eigen::Vector3f toDirection(float u, float v) {
        float phi = u * 2.f * (float)M_PI;
        float cosTheta = 1.f - 2.f * v;
        float sinTheta = sqrtf(std::max(0.0f, 1.0f - cosTheta * cosTheta));
        return { sinTheta * cosf(phi), cosTheta, sinTheta * sinf(phi) };
    }

    /**
     * @brief Inverse of @ref toDirection : unit direction to @c (u, v) .
     * @param d Unit-length direction vector.
     * @return @c (u, v) with @c v the equal-area polar coordinate.
     */
    inline Eigen::Vector2f fromDirection(const Eigen::Vector3f& d) {
        float u = atan2f(d.z(), d.x()) / (2.f * (float)M_PI);
        if (u < 0.f) u += 1.f;
        float v = std::clamp((1.f - d.y()) * 0.5f, 0.f, 1.f);
        return { u, v };
    }

    /// @brief Total solid angle of the unit sphere, \f$4\pi\f$ steradians.
    inline constexpr float SOLID_ANGLE_FACTOR = 4.0f * (float)M_PI;
}
