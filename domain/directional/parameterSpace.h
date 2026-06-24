#pragma once

#include "../projection.h"

#include <utility>

// Coordinate representation a directional distribution can be expressed in.
// Canonical pivot is equal-area (Quadtree): only v warps between spaces, u is
// phi = 2*pi*u in all three. Spherical matches a Mitsuba spherical sensor
// (equirectangular, 2:1 lat-long image).
enum class ParameterSpace { Quadtree, BTC, Spherical };

inline const char* parameterSpaceName(ParameterSpace s) {
    switch (s) {
        case ParameterSpace::Quadtree:  return "Equal-area (Quadtree)";
        case ParameterSpace::BTC:       return "Equirectangular (BTC)";
        case ParameterSpace::Spherical: return "Spherical (Mitsuba)";
    }
    return "?";
}

// Output grid (width, height) for a base resolution. Spherical is 2:1.
inline std::pair<int, int> parameterSpaceDims(ParameterSpace s, int baseRes) {
    if (s == ParameterSpace::Spherical) return { 2 * baseRes, baseRes };
    return { baseRes, baseRes };
}

// space v -> canonical (equal-area) v.
inline float parameterSpaceVToCanonical(ParameterSpace s, float v) {
    switch (s) {
        case ParameterSpace::Quadtree:  return v;
        case ParameterSpace::BTC:
        case ParameterSpace::Spherical: return EquirectToEqualArea::inverse(v);
    }
    return v;
}

// canonical (equal-area) v -> space v.
inline float parameterSpaceVFromCanonical(ParameterSpace s, float v) {
    switch (s) {
        case ParameterSpace::Quadtree:  return v;
        case ParameterSpace::BTC:
        case ParameterSpace::Spherical: return EquirectToEqualArea::forward(v);
    }
    return v;
}

// Solid angle subtended by one pixel at vertical coord v, for integral checks.
inline float parameterSpaceSolidAngleElement(ParameterSpace s, float v, int w, int h) {
    if (s == ParameterSpace::Quadtree)
        return CylindricalEqualArea::SOLID_ANGLE_FACTOR / (float)(w * h);
    return EquirectToEqualArea::jacobian(v) / (float)(w * h);
}

// A spherical-image pixel (px,py in [0,1]) -> the world direction the exported
// Mitsuba spherical camera samples there. Mitsuba's sensor maps the pixel to a
// local direction (sinθcosφ, sinθsinφ, cosθ) with θ=π·py, φ=2π·px; the camera's
// lookat (forward +X, up +Z, from the export feature) rotates that to world
// (cosθ, sinθcosφ, sinθsinφ). VERIFY: depends on Mitsuba's lookat handedness.
inline Eigen::Vector3f sphericalPixelToDirection(float px, float py) {
    float theta = (float)M_PI * py;
    float phi   = 2.f * (float)M_PI * px;
    float st = std::sin(theta), ct = std::cos(theta);
    return { ct, st * std::cos(phi), st * std::sin(phi) };
}

// Normalised grid pixel (fx,fy; row 0 = bottom scanline) -> dump (u, equal-area v).
inline Eigen::Vector2f parameterSpacePixelToDump(ParameterSpace s, float fx, float fy) {
    if (s == ParameterSpace::Spherical)
        return CylindricalEqualArea::fromDirection(sphericalPixelToDirection(fx, 1.f - fy));
    return { fx, parameterSpaceVToCanonical(s, fy) };
}

// Normalised grid pixel -> 3D direction (dump frame) for the hover ray.
inline Eigen::Vector3f parameterSpacePixelToDirection(ParameterSpace s, float fx, float fy) {
    if (s == ParameterSpace::Spherical)
        return sphericalPixelToDirection(fx, 1.f - fy);
    return CylindricalEqualArea::toDirection(fx, parameterSpaceVToCanonical(s, fy));
}
