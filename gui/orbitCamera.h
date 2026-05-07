#pragma once

#include <Eigen/Dense>

/**
 * @brief  Orbit camera rotates around @c target at @c distance , with affine pan offsets.
 * 
 */
 class OrbitCamera {
public:
    Eigen::Vector3f target{ 0, 0, 0 };
    float           distance  = 3;
    float           azimuth   = 0.7f;
    float           elevation = 0.5f;

    Eigen::Matrix4f viewMatrix() const;

    static Eigen::Matrix4f perspective(float fov, float aspect, float zNear, float zFar);

    void orbit(float da, float de);
    void pan(float dx, float dy);
    void zoom(float d);
};
