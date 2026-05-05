#include "appState.h"

#include <type_traits>
#include <variant>

int AppState::getSelectedCellA() const {
    return std::visit([](auto const& s) -> int {
        using T = std::decay_t<decltype(s)>;
        if      constexpr (std::is_same_v<T, NormalState>)         return s.selectedCellA;
        else if constexpr (std::is_same_v<T, PartialCompState>)    return s.selectedCellA;
        else if constexpr (std::is_same_v<T, TotalCompState>)      return s.selectedCellA;
        else if constexpr (std::is_same_v<T, IterationDiffState>)  return s.parentCellIdx;
        else                                                       return -1;
    }, mode);
}

int AppState::getSelectedCellB() const {
    if (auto* n = asMode<NormalState>()) return n->selectedCellB;
    return -1;
}

void AppState::setSelectedCellAIndex(int idx) {
    std::visit([idx](auto& s) {
        using T = std::decay_t<decltype(s)>;
        if      constexpr (std::is_same_v<T, NormalState>)         s.selectedCellA = idx;
        else if constexpr (std::is_same_v<T, PartialCompState>)    s.selectedCellA = idx;
        else if constexpr (std::is_same_v<T, TotalCompState>)      s.selectedCellA = idx;
        else if constexpr (std::is_same_v<T, IterationDiffState>)  s.parentCellIdx = idx;
    }, mode);
}
