#include "sdtreeSpatial.h"

#include "../directional/directionalLoader.h"

#include <algorithm>
#include <iostream>

bool STreeNode::read(BlobReader& b, DirKind directionalType) {
    b >> isLeaf >> axis >> children[0] >> children[1];
    if (isLeaf) {
        dTree = loadDirectional(b, directionalType);
        if (!dTree) return false;
    }
    return b.isValid();
}

bool SDTree::load(const std::string& fn, DirKind directionalKind) {
    BlobReader r(fn);
    if (!r.isValid()) return false;

    header = r.readString();
    type   = directionalKind;

    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            r >> cameraMatrix(i, j);

    std::size_t nn;
    r >> nn
      >> aabbMin.x() >> aabbMin.y() >> aabbMin.z()
      >> aabbMax.x() >> aabbMax.y() >> aabbMax.z();

    treeNodes.resize(nn);
    for (std::size_t i = 0; i < nn; ++i) {
        if (!treeNodes[i].read(r, type)) return false;
    }

    nodes.clear();
    traverseAndFlatten(0, aabbMin, aabbMax - aabbMin);

    for (int i = 0; i < (int)nodes.size(); ++i)
        nodes[i].index = i;

    name = fn;
    auto sl = name.find_last_of("/\\"); if (sl != std::string::npos) name = name.substr(sl + 1);
    auto dt = name.find_last_of('.');   if (dt != std::string::npos) name = name.substr(0, dt);

    const char* typeLabel = (type == BTC)          ? "BTC"
                          : (type == CostQuadTree) ? "DTree+Costs"
                                                    : "DTree";
    std::cout << "Loaded " << nodes.size() << " leaves ["
              << typeLabel << "] " << fn << std::endl;

    return true;
}

void SDTree::traverseAndFlatten(uint32_t idx, Eigen::Vector3f pos, Eigen::Vector3f size) {
    auto& n = treeNodes[idx];
    if (n.isLeaf) {
        n.leafIndex = (int)nodes.size();

        SpatialNode sn;
        sn.pos          = pos + size * 0.5f;
        sn.size         = size;
        sn.meanRadiance = n.dTree->getMean();
        sn.nSamples     = n.dTree->getNumSamples();
        sn.dTree        = n.dTree;

        nodes.push_back(sn);
    } else {
        int             ax       = n.axis;
        Eigen::Vector3f halfSize = size;
        halfSize[ax] *= 0.5f;

        Eigen::Vector3f secondPos = pos;
        secondPos[ax] += halfSize[ax];

        traverseAndFlatten(n.children[0], pos, halfSize);
        traverseAndFlatten(n.children[1], secondPos, halfSize);
    }
}
