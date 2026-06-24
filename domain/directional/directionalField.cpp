#include "directionalField.h"

#include <algorithm>

DirectionalField DirectionalField::toParameterSpace(ParameterSpace target) const {
    if (target == space) return *this;

    DirectionalField out;
    out.space = target;
    out.leaves.reserve(leaves.size());

    for (const auto& leaf : leaves) {
        float canMinB = parameterSpaceVToCanonical(space, leaf.bounds.minB);
        float canMaxB = parameterSpaceVToCanonical(space, leaf.bounds.maxB);
        float tMinB   = parameterSpaceVFromCanonical(target, canMinB);
        float tMaxB   = parameterSpaceVFromCanonical(target, canMaxB);
        if (tMaxB < tMinB) std::swap(tMinB, tMaxB);

        out.leaves.push_back({
            leaf.radiance,
            Bounds2f{ leaf.bounds.minA, tMinB, leaf.bounds.maxA, tMaxB }
        });
    }
    return out;
}
