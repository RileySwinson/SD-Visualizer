#pragma once

#include "perModeStates.h"

#include <variant>

// The mode of the viewer is encoded as a sum type. Mode-specific state lives
// inside the relevant arm — invalid states (e.g. TotalCompState without a
// sequence) are unrepresentable.
//
// TotalCompState and IterationDiffState are only constructed from a Sequence
// context; the type itself doesn't enforce that, but SDTreeViewer's
// transition methods (enterTotalDiff, enterIterationMode) do.
using Mode = std::variant<
    NormalState,
    PartialCompState,
    TotalCompState,
    IterationDiffState
>;
