#include "filter.h"

std::vector<int> computeFilteredIndices(const SDTree& dataset, const Eigen::AlignedBox3f& filterBox)
{
    std::vector<int> result;
    
    for (int i = 0; i < (int)dataset.nodes.size(); ++i) {
        auto& node = dataset.nodes[i];
        Eigen::AlignedBox3f nodeBox(node.pos - node.size * 0.5f, node.pos + node.size * 0.5f);
        if (filterBox.intersects(nodeBox)) result.push_back(i);
    }

    return result;
}

float filteredTotalRadiance(const SDTree& dataset, const Eigen::AlignedBox3f& filterBox, bool applyFilter)
{
    if (!applyFilter) {
        float total = 0;
        for (auto& node : dataset.nodes) total += node.meanRadiance;
        return total;
    }

    float total = 0;
    for (auto& node : dataset.nodes) {
        Eigen::AlignedBox3f nodeBox(node.pos - node.size * 0.5f, node.pos + node.size * 0.5f);
        if (filterBox.intersects(nodeBox)) total += node.meanRadiance;
    }
    return total;
}
