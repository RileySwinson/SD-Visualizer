#include "pointCloudBuild.h"

#include "../../globals/config.h"
#include "../../globals/palette.h"

#include <algorithm>
#include <cmath>

PointCloudResult buildPointCloud(
    const SDTree& dataset,
    const std::vector<int>& visibleIndices,
    const std::vector<DivStatus>* divStatus
) {
    PointCloudResult out;
    out.sceneMin = Eigen::Vector3f( 1e30f,  1e30f,  1e30f);
    out.sceneMax = Eigen::Vector3f(-1e30f, -1e30f, -1e30f);

    if (visibleIndices.empty()) return out;

    float radMin = 1e30f, radMax = -1e30f;
    for (int i : visibleIndices) {
        auto& node = dataset.nodes[i];
        out.sceneMin = out.sceneMin.cwiseMin(node.pos - node.size * 0.5f);
        out.sceneMax = out.sceneMax.cwiseMax(node.pos + node.size * 0.5f);

        float r = std::max(node.meanRadiance, PDF_FLOOR);
        radMin = std::min(radMin, r);
        radMax = std::max(radMax, r);
    }

    if (radMin >= radMax) radMax = radMin * 10;

    float logMin   = std::log(radMin);
    float logMax   = std::log(radMax);
    float logRange = std::max(logMax - logMin, 1e-6f);

    out.vertices.resize(visibleIndices.size());

    for (std::size_t vi = 0; vi < visibleIndices.size(); ++vi) {
        int    idx  = visibleIndices[vi];
        auto&  node = dataset.nodes[idx];

        float t = (std::log(std::max(node.meanRadiance, radMin)) - logMin) / logRange;
        t = std::max(0.f, std::min(1.f, t));

        Color3f color;
        if (divStatus != nullptr && idx < (int)divStatus->size()) {
            auto status = (*divStatus)[idx];
            if      (status == DIV_REFINED)   color = { 1.f, 0.5f, 0.f  };
            else if (status == DIV_DIVERGENT) color = { 1.f, 0.1f, 0.1f };
            else                              color = sampleColormap(CMAP_PDF, t);
        } else {
            color = sampleColormap(CMAP_PDF, t);
        }

        out.vertices[vi] = {
            node.pos.x(), node.pos.y(), node.pos.z(),
            color.r, color.g, color.b, 0.85f
        };
    }

    return out;
}
