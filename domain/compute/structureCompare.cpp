#include "structureCompare.h"

namespace {

void markSubtree(
    int index,
    const std::vector<STreeNode>& nodes,
    DivStatus status,
    std::vector<DivStatus>& out
) {
    if (index < 0 || index >= (int)nodes.size()) return;

    auto& n = nodes[index];
    if (n.isLeaf) {
        if (n.leafIndex >= 0 && n.leafIndex < (int)out.size()) {
            out[n.leafIndex] = status;
        }
    } else {
        markSubtree(n.children[0], nodes, status, out);
        markSubtree(n.children[1], nodes, status, out);
    }
}

void walkCompare(
    int oldIndex, int newIndex,
    const std::vector<STreeNode>& oldNodes,
    const std::vector<STreeNode>& newNodes,
    std::vector<DivStatus>& out
) {
    if (oldIndex < 0 || oldIndex >= (int)oldNodes.size()
     || newIndex < 0 || newIndex >= (int)newNodes.size()) return;

    auto& oldNode = oldNodes[oldIndex];
    auto& newNode = newNodes[newIndex];

    // Both leaves: unchanged.
    // Old leaf, new internal: refined (split between iterations).
    // Old internal, new leaf: divergent (collapse happened. this should not happen)
    // Both internal: recurse if the axis matches, otherwise its a divergent subtree.
    if (oldNode.isLeaf && newNode.isLeaf) {
        if (newNode.leafIndex >= 0 && newNode.leafIndex < (int)out.size()) {
            out[newNode.leafIndex] = DIV_UNCHANGED;
        }
    } else if (oldNode.isLeaf && !newNode.isLeaf) {
        markSubtree(newIndex, newNodes, DIV_REFINED, out);
    } else if (!oldNode.isLeaf && newNode.isLeaf) {
        if (newNode.leafIndex >= 0 && newNode.leafIndex < (int)out.size())
            out[newNode.leafIndex] = DIV_DIVERGENT;
    } else {
        if (oldNode.axis == newNode.axis) {
            walkCompare(oldNode.children[0], newNode.children[0], oldNodes, newNodes, out);
            walkCompare(oldNode.children[1], newNode.children[1], oldNodes, newNodes, out);
        } else {
            markSubtree(newIndex, newNodes, DIV_DIVERGENT, out);
        }
    }
}

} // namespace

DivisionResult compareStructure(const SDTree& older, const SDTree& newer) {
    DivisionResult result;
    result.status.resize(newer.nodes.size(), DIV_UNCHANGED);

    if (!older.treeNodes.empty() && !newer.treeNodes.empty()) {
        walkCompare(0, 0, older.treeNodes, newer.treeNodes, result.status);
    }

    for (auto s : result.status) {
        if      (s == DIV_UNCHANGED) result.numUnchanged++;
        else if (s == DIV_REFINED)   result.numRefined++;
        else                         result.numDivergent++;
    }

    result.valid = true;
    return result;
}
