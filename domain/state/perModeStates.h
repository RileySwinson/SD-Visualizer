#pragma once

#include "../compute/structureCompare.h"

#include <vector>

// Per-mode state arms for the Mode variant.
//
// These hold only mode-specific bookkeeping, mainly  the index-based
// selection state. User-bound input fields like XYZ coordinates live at AppState so they survive mode changes

// NORMAL: single-dataset selection with optional Cell B for KL comparison.
struct NormalState {
    int selectedCellA = 0;
    int selectedCellB = -1;
    bool isKlMode = false;
};

// PARTIAL_COMP: select N datasets and compare them at a given coordinate.
// Anchor coord and dsChecked live in AppState.
struct PartialCompState {
    int selectedCellA = 0; // for the active-dataset spatial highlight
};

// TOTAL_COMP: structural diff between two iterations of one sequence.
struct TotalCompState {
    int diffBefore = -1;
    int diffAfter = -1;
    DivisionResult divResult;
    int selectedCellA = 0;
};

// ITERATION_DIFF: parent cell in iter A vs descendants in iter B.
struct IterationDiffState {
    int groupIndex = -1;
    int iterA = -1; // older iteration 
    int iterB = -1; // newer iteration (dataset index)
    int parentCellIdx = 0;  // selected parent cell in iter A
    std::vector<int> childrenIndices;
    int  activeChild = 0;
    bool showAll = false;

    static const int MAX_SHOW_ALL  = 6;
};
