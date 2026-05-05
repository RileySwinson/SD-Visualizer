#include "nodeQueries.h"

int findNode(const SDTree& tree, const Eigen::Vector3f& point) {
    int   containing = -1;
    float smallestVolume = 1e30f;

    for (int i = 0; i < (int)tree.nodes.size(); ++i) {
        auto& node = tree.nodes[i];
        Eigen::Vector3f halfSize = node.size * 0.5f;
        Eigen::Vector3f lo       = node.pos - halfSize;
        Eigen::Vector3f hi       = node.pos + halfSize;

        bool inside = (point.x() >= lo.x() && point.x() <= hi.x())
                   && (point.y() >= lo.y() && point.y() <= hi.y())
                   && (point.z() >= lo.z() && point.z() <= hi.z());

        if (inside) {
            float v = node.size.x() * node.size.y() * node.size.z();
            if (v < smallestVolume) {
                smallestVolume = v;
                containing = i;
            }
        }
    }

    if (containing >= 0) return containing;

    // Fallback is nearest leaf by centroid distance.
    int   nearest = 0;
    float nearestDistSq = 1e30f;
    for (int i = 0; i < (int)tree.nodes.size(); ++i) {
        float d = (tree.nodes[i].pos - point).squaredNorm();
        if (d < nearestDistSq) { nearestDistSq = d; nearest = i; }
    }
    return nearest;
}

const DirLeaf* findLeafUV(const std::vector<DirLeaf>& leaves, float u, float v) {
    const DirLeaf* best     = nullptr;
    float          bestArea = 1e30f;

    for (auto& leaf : leaves) {
        if (u >= leaf.bounds.minA && u <= leaf.bounds.maxA
         && v >= leaf.bounds.minB && v <= leaf.bounds.maxB)
        {
            float area = (leaf.bounds.maxA - leaf.bounds.minA) * (leaf.bounds.maxB - leaf.bounds.minB);
            if (area < bestArea) { bestArea = area; best = &leaf; }
        }
    }
    return best;
}

std::vector<int> findDescendants(const SDTree& newerTree, const Eigen::Vector3f& parentPos, const Eigen::Vector3f& parentSize)
{
    std::vector<int> result;
    Eigen::Vector3f  parentMin = parentPos - parentSize * 0.5f;
    Eigen::Vector3f  parentMax = parentPos + parentSize * 0.5f;
    const float eps = 1e-5f;

    for (int i = 0; i < (int)newerTree.nodes.size(); ++i) {
        auto& node = newerTree.nodes[i];
        Eigen::Vector3f nMin = node.pos - node.size * 0.5f;
        Eigen::Vector3f nMax = node.pos + node.size * 0.5f;

        if (nMin.x() >= parentMin.x() - eps && nMin.y() >= parentMin.y() - eps && nMin.z() >= parentMin.z() - eps
         && nMax.x() <= parentMax.x() + eps && nMax.y() <= parentMax.y() + eps && nMax.z() <= parentMax.z() + eps)
        {
            result.push_back(i);
        }
    }
    return result;
}
