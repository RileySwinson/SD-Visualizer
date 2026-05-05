#include "probe.h"

#include "../blobReader.h"

#include <algorithm>

DumpDescriptor probeDump(const std::string& fn) {
    DumpDescriptor dump;

    BlobReader r(fn);
    if (!r.isValid()) return dump;

    dump.spatial = SpatialKind::SDTree;

    std::string fnLower = fn;
    std::transform(fnLower.begin(), fnLower.end(), fnLower.begin(), ::tolower);

    // Detect directional structures by filename substring.
    if (fnLower.find("btc") != std::string::npos) {
        dump.dirKind = DirKind::BTC;
    } else if (fnLower.find("costs") != std::string::npos) {
        dump.dirKind = DirKind::CostQuadTree;
    } else {
        dump.dirKind = DirKind::QuadTree;
    }

    dump.valid = true;
    return dump;
}
