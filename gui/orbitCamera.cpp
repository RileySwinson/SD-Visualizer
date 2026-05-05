#include "orbitCamera.h"

#include <algorithm>
#include <cmath>

// just boring normal camera stuff 

Eigen::Matrix4f OrbitCamera::viewMatrix() const {
    float cosAz = std::cos(azimuth),   sinAz = std::sin(azimuth);
    float cosEl = std::cos(elevation), sinEl = std::sin(elevation);

    Eigen::Vector3f eye   = target + distance * Eigen::Vector3f(cosEl * sinAz, sinEl, cosEl * cosAz);
    Eigen::Vector3f up(0, 1, 0);
    Eigen::Vector3f fwd   = (target - eye).normalized();
    Eigen::Vector3f right = fwd.cross(up).normalized();
    up = right.cross(fwd); // reorthogonalise

    Eigen::Matrix4f V = Eigen::Matrix4f::Identity();
    V(0, 0) = right.x(); V(0, 1) = right.y(); V(0, 2) = right.z();
    V(1, 0) = up.x();    V(1, 1) = up.y();    V(1, 2) = up.z();
    V(2, 0) = -fwd.x();  V(2, 1) = -fwd.y();  V(2, 2) = -fwd.z();
    V(0, 3) = -right.dot(eye); V(1, 3) = -up.dot(eye); V(2, 3) = fwd.dot(eye);
    return V;
}

Eigen::Matrix4f OrbitCamera::perspective(float fov, float aspect, float zNear, float zFar) {
    float f = 1 / std::tan(fov * 0.5f);
    Eigen::Matrix4f P = Eigen::Matrix4f::Zero();

    P(0, 0) = f / aspect; P(1, 1) = f;
    P(2, 2) = (zFar + zNear) / (zNear - zFar);
    P(2, 3) = (2 * zFar * zNear) / (zNear - zFar);
    P(3, 2) = -1;
    return P;
}

void OrbitCamera::orbit(float da, float de) {
    azimuth   += da;
    elevation  = std::max(-1.5f, std::min(1.5f, elevation + de));
}

void OrbitCamera::pan(float dx, float dy) {
    float cosAz = std::cos(azimuth),   sinAz = std::sin(azimuth);
    float cosEl = std::cos(elevation), sinEl = std::sin(elevation);

    Eigen::Vector3f eye   = target + distance * Eigen::Vector3f(cosEl * sinAz, sinEl, cosEl * cosAz);
    Eigen::Vector3f fwd   = (target - eye).normalized();
    Eigen::Vector3f right = fwd.cross(Eigen::Vector3f(0, 1, 0)).normalized();
    Eigen::Vector3f up    = right.cross(fwd);

    target += right * dx * distance * 0.002f + up * dy * distance * 0.002f;
}

void OrbitCamera::zoom(float d) {
    distance *= (1 - d * 0.1f);
    distance  = std::max(0.01f, distance);
}
