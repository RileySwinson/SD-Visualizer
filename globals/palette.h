#pragma once

#include <vector>
#include <algorithm>

struct Color3f { float r, g, b; };

inline Color3f lerpColor(const Color3f& a, const Color3f& b, float t) {
    t = std::max(0.f, std::min(1.0f, t));
    return {
        a.r * (1 - t) + b.r * t,
        a.g * (1 - t) + b.g * t,
        a.b * (1 - t) + b.b * t,
    };
}

inline Color3f sampleColormap(const std::vector<Color3f>& s, float t) {
    t = std::max(0.f, std::min(1.0f, t));
    float tScaled = t * (s.size() - 1);
    int   tScaledInt = (int)tScaled;
    return lerpColor(
        s[tScaledInt],
        s[std::min(tScaledInt + 1, (int)s.size() - 1)],
        tScaled - tScaledInt
    );
}

// Inferno colormap, sampled from matplotlib
inline const std::vector<Color3f> CMAP_PDF = {
    {0.001f, 0.000f, 0.014f}, {0.120f, 0.047f, 0.282f},
    {0.288f, 0.053f, 0.419f}, {0.440f, 0.075f, 0.393f},
    {0.577f, 0.148f, 0.311f}, {0.705f, 0.241f, 0.215f},
    {0.817f, 0.357f, 0.122f}, {0.907f, 0.497f, 0.039f},
    {0.965f, 0.655f, 0.047f}, {0.979f, 0.822f, 0.213f},
    {0.988f, 0.998f, 0.645f}
};

// Viridis colormap, sampled from matplotlib
inline const std::vector<Color3f> CMAP_KL = {
    {0.267f, 0.004f, 0.329f}, {0.283f, 0.141f, 0.458f},
    {0.254f, 0.265f, 0.530f}, {0.207f, 0.372f, 0.553f},
    {0.164f, 0.471f, 0.558f}, {0.128f, 0.567f, 0.551f},
    {0.135f, 0.659f, 0.518f}, {0.267f, 0.749f, 0.441f},
    {0.478f, 0.821f, 0.318f}, {0.741f, 0.873f, 0.150f},
    {0.993f, 0.906f, 0.144f}
};
