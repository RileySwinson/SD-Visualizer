#pragma once

#include <Eigen/Dense>
#include <algorithm>

// 2D bounding rectangle in float space.
// Used in multiple coordinate spaces, so axes are labelled A, B instead of U, V or X, Y.
struct Bounds2f {
    float minA;
    float minB;
    float maxA;
    float maxB;

    static constexpr Bounds2f unit() { return Bounds2f{0, 0, 1, 1}; }

    float width()  const { return maxA - minA; }
    float height() const { return maxB - minB; }
    float area()   const { return width() * height(); }

    inline float midA() { return (minA + maxA) * 0.5f; }
    inline float midB() { return (minB + maxB) * 0.5f; }

    Eigen::Vector2f center() const {
        return { (minA + maxA) * 0.5f, (minB + maxB) * 0.5f };
    }

    bool contains(float x, float y) const {
        return x >= minA && x <= maxA && y >= minB && y <= maxB;
    }

    bool doesOverlapWith(const Bounds2f& b) const {
        return minA <= b.maxA && maxA >= b.minA && minB <= b.maxB && maxB >= b.minB;
    }

    Bounds2f clampTo(float lower, float upper) const {
        return {
            std::max(lower, minA), std::max(lower, minB),
            std::min(upper, maxA), std::min(upper, maxB)
        };
    }

    bool isEmpty() const { return maxA <= minA || maxB <= minB; }
};

// Integer counterpart of Bounds2f, suitable for pixel-loop iteration.
struct Bounds2i {
    int minA;
    int minB;
    int maxA;
    int maxB;

    static Bounds2i fromBounds2f(const Bounds2f& b, int resolution) {
        return fromBounds2f(b, resolution, resolution);
    }

    static Bounds2i fromBounds2f(const Bounds2f& b, int w, int h) {
        return {
            std::max(0, (int)(b.minA * w)),
            std::max(0, (int)(b.minB * h)),
            std::min(w, (int)(b.maxA * w)),
            std::min(h, (int)(b.maxB * h))
        };
    }

    bool isEmpty() const { return maxA <= minA || maxB <= minB; }
};
