#pragma once

#include <GL/glew.h>
#include <Eigen/Dense>

#include <array>
#include <vector>

// 12 line segments forming the wireframe box of (center +/- size/2).
void boxEdges(
    const Eigen::Vector3f& center, const Eigen::Vector3f& size,
    std::vector<Eigen::Vector3f>& out
);

// Same but parameterised by min/max corners.
void boxEdgesMinMax(
    const Eigen::Vector3f& lo, const Eigen::Vector3f& hi,
    std::vector<Eigen::Vector3f>& out
);

// Upload `points` (consecutive pairs = line segments) and draw.
// The line program is bound and a uniform color is applied per call.
void drawLines(
    GLuint lineProg, GLuint lineVAO, GLuint lineVBO,
    const std::vector<Eigen::Vector3f>& points,
    const Eigen::Matrix4f& mvp,
    std::array<float, 4> color, float lineWidth
);

// Convenience wrapper: 12 edges of an AABB drawn as a wireframe box.
void drawBox(
    GLuint lineProg, GLuint lineVAO, GLuint lineVBO,
    const Eigen::Vector3f& center, const Eigen::Vector3f& size,
    const Eigen::Matrix4f& mvp,
    std::array<float, 4> color, float lineWidth
);
