#pragma once

#include "binInfo.h"
#include "displayFlags.h"
#include "mode.h"
#include "slot.h"

#include "../../globals/config.h"
#include "../spatial/sdtreeSpatial.h"

#include <Eigen/Dense>

#include <string>
#include <vector>

struct KLSelection {
    bool isActive = false;
    int  slotA  = -1;
    int  slotB = -1;
};

struct HoverRay {
    bool active = false;
    Eigen::Vector3f origin{ 0, 0, 0 };
    Eigen::Vector3f direction{ 0, 0, 0 };
};

/**
 * @brief The source of truth for the viewer's runtime state. 
 * created on viewer startup with @p datasets populated, mutated by SDTreeViewer methods invoked from panels and keyboard input.
 * 
 */
struct AppState {
    std::vector<SDTree> datasets;
    int                 activeDataset = 0;
    std::string         prefix;

    DisplayFlags flags;
    Mode mode = NormalState{};

    // Recomputed when (activeDataset, flags.filter*) change.
    std::vector<int> filteredIndices;

    // Persists across mode toggles.
    Eigen::Vector3f cellAXYZ{ 0, 0, 0 };
    Eigen::Vector3f cellBXYZ{ 0, 0, 0 };
    Eigen::Vector3f partialXYZ{ 0, 0, 0 };
    std::vector<bool> dsChecked;

    // Heatmap slots.
    Slot slots[MAX_COMP_SLOTS];
    int  numSlots = 0;
    int  slotTiling[MAX_COMP_SLOTS] = { -1, -1, -1, -1, -1, -1, -1, -1 };

    KLSelection kl;
    BinInfo   insp;
    HoverRay  hover;

    // Frame-local input edge-detection state
    // MAX_KEYS is hard-coded to avoid pulling GLFW into this header, which
    // would force GLEW include for anything that uses AppState.
    // i dont like these kind of magic numbers though
    static constexpr int MAX_KEYS = 512;
    bool isShiftHeld = false;
    bool prevKeys[MAX_KEYS] = {};

    // Mode-agnostic accessors over the active variant arm.
    int  getSelectedCellA() const; // -1 if not meaningful
    int  getSelectedCellB() const; // -1 if not in NormalState
    void setSelectedCellAIndex(int idx); 

    template <class T> bool isMode() const { return std::holds_alternative<T>(mode); }
    template <class T> T* asMode() { return std::get_if<T>(&mode); }
    template <class T> const T* asMode() const { return std::get_if<T>(&mode); }
};
