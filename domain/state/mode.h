#pragma once

#include "perModeStates.h"

#include <variant>

// Mode-specific state lives inside the relevant code, and invalid states are unrepresentable.
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
