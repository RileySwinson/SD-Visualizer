#include "settingsPanel.h"

#include "../../viewer/sdtreeViewer.h"
#include "../../globals/uiColors.h"
#include "../../domain/directional/parameterSpace.h"

#include <imgui.h>

#include <cstdio>
#include <string>

using std::string;
using std::to_string;

void renderSettingsPanel(SDTreeViewer& viewer) {
    auto& state = viewer.state;

    ImGui::TextColored(COL_TEXT, "Mode:");
    if (ImGui::RadioButton("Normal", state.isMode<NormalState>())) {
        viewer.enterNormalMode();
    }
    if (ImGui::RadioButton("Partial Diff", state.isMode<PartialCompState>())) {
        viewer.enterPartialMode();
    }
    {
        auto seqGroups = viewer.getSequenceGroups();
        if (!seqGroups.empty()) {
            if (ImGui::RadioButton("Sequence", state.isMode<IterationDiffState>())) {
                viewer.enterIterationMode(seqGroups[0]);
            }
        }
    }
    ImGui::Separator();

    ImGui::TextColored(COL_TEXT, "Directional space:");
    ImGui::SetNextItemWidth(-1);
    if (ImGui::BeginCombo("##paramSpace", parameterSpaceName(state.flags.paramSpace))) {
        for (ParameterSpace ps : { ParameterSpace::Quadtree,
                                   ParameterSpace::BTC,
                                   ParameterSpace::Spherical }) {
            bool sel = (state.flags.paramSpace == ps);
            if (ImGui::Selectable(parameterSpaceName(ps), sel) && !sel) {
                state.flags.paramSpace = ps;
                viewer.updateHeatmaps();
            }
            if (sel) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    if (state.flags.paramSpace == ParameterSpace::Spherical)
        ImGui::TextColored({ 0.5f, 0.5f, 0.5f, 1 },
            "2:1 lat-long, matches Mitsuba spherical sensor");

    ImGui::TextColored(COL_TEXT, "Heatmap scale:");
    ImGui::SetNextItemWidth(-1);
    if (ImGui::BeginCombo("##heatScale", heatmapScaleName(state.flags.heatmapScale))) {
        for (HeatmapScale hs : { HeatmapScale::FromZero, HeatmapScale::AutoRange }) {
            bool sel = (state.flags.heatmapScale == hs);
            if (ImGui::Selectable(heatmapScaleName(hs), sel) && !sel) {
                state.flags.heatmapScale = hs;
                viewer.updateHeatmaps();
            }
            if (sel) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    ImGui::Separator();

    if (auto* it = state.asMode<IterationDiffState>()) {
        auto seqGroups = viewer.getSequenceGroups();

        ImGui::TextColored(COL_TEXT, "Sequence:");
        if (ImGui::BeginCombo("##SeqGroup",
            it->groupIndex >= 0 ? ("Group " + to_string(it->groupIndex)).c_str() : "Select..."))
        {
            for (int g : seqGroups) {
                auto members = viewer.getGroupMembers(g);
                string label = "Group " + to_string(g);
                if (!members.empty()) {
                    string gPrefix = state.datasets[members[0]].name;
                    for (int mi = 1; mi < (int)members.size(); ++mi) {
                        string& other = state.datasets[members[mi]].name;
                        while (gPrefix.size() > 0 && other.substr(0, gPrefix.size()) != gPrefix) gPrefix.pop_back();
                    }
                    if (!gPrefix.empty()) label = gPrefix + " (" + to_string(members.size()) + " iters)";
                }
                if (ImGui::Selectable(label.c_str(), it->groupIndex == g)) {
                    viewer.setSequenceGroup(g);
                }
            }
            ImGui::EndCombo();
        }

        if (it->groupIndex >= 0) {
            auto members = viewer.getGroupMembers(it->groupIndex);
            string gPrefix = state.datasets[members[0]].name;
            for (int mi = 1; mi < (int)members.size(); ++mi) {
                string& other = state.datasets[members[mi]].name;
                while (gPrefix.size() > 0 && other.substr(0, gPrefix.size()) != gPrefix) gPrefix.pop_back();
            }

            ImGui::TextColored(COL_TEXT, "Iterations:");
            ImGui::TextColored({ 0.5f, 0.5f, 0.5f, 1 }, "Click=Iter A, Shift=Iter B");
            for (int mi = 0; mi < (int)members.size(); ++mi) {
                int dsIdx = members[mi];
                string suffix = state.datasets[dsIdx].name.substr(gPrefix.size());
                if (suffix.empty()) suffix = to_string(mi + 1);

                bool isA = (dsIdx == it->iterA);
                bool isB = (dsIdx == it->iterB);
                if      (isA) ImGui::PushStyleColor(ImGuiCol_Button, { COL_ORANGE.x, COL_ORANGE.y, COL_ORANGE.z, 0.7f });
                else if (isB) ImGui::PushStyleColor(ImGuiCol_Button, { COL_PURPLE.x, COL_PURPLE.y, COL_PURPLE.z, 0.7f });

                char btnId[64];
                std::snprintf(btnId, sizeof(btnId), "%s##seq_%d", suffix.c_str(), mi);
                if (ImGui::Button(btnId, { -1, 0 })) {
                    viewer.clickIterationButton(dsIdx, state.isShiftHeld);
                }
                if (isA || isB) ImGui::PopStyleColor();
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::Text("%s", state.datasets[dsIdx].name.c_str());
                    ImGui::EndTooltip();
                }
            }

            if (it->iterA >= 0) ImGui::TextColored(COL_ORANGE, "A: %s", state.datasets[it->iterA].name.c_str());
            if (it->iterB >= 0) ImGui::TextColored(COL_PURPLE, "B: %s", state.datasets[it->iterB].name.c_str());

            ImGui::Separator();

            bool canShowAll = !it->childrenIndices.empty()
                           && (int)it->childrenIndices.size() <= IterationDiffState::MAX_SHOW_ALL;
            if (!canShowAll && it->showAll) it->showAll = false;
            if (!canShowAll) ImGui::BeginDisabled();
            bool showAllValue = it->showAll;
            if (ImGui::Checkbox("Show heatmaps for all child cells", &showAllValue)) {
                viewer.toggleShowAllChildren(showAllValue);
            }
            if (!canShowAll) {
                ImGui::EndDisabled();
                if (!it->childrenIndices.empty()) {
                    ImGui::TextColored({ 0.5f, 0.5f, 0.5f, 1 },
                        "(%d children, max %d for show-all)",
                        (int)it->childrenIndices.size(), IterationDiffState::MAX_SHOW_ALL);
                }
            }

            if (!it->childrenIndices.empty()) {
                ImGui::TextColored(COL_TEXT, "%d descendant(s)", (int)it->childrenIndices.size());
            }
        }

        ImGui::Separator();
        ImGui::TextColored(COL_ORANGE, "Cell A:");
        ImGui::SetNextItemWidth(-1); ImGui::InputFloat3("##cA", state.cellAXYZ.data(), "%.3f");
        if (ImGui::Button("Go##A", { -1, 0 })) viewer.goToCellA();
    } else if (state.isMode<PartialCompState>()) {
        ImGui::TextColored(COL_TEXT, "Datasets:");
        for (int i = 0; i < (int)state.datasets.size(); ++i) {
            if (i >= (int)state.dsChecked.size()) state.dsChecked.push_back(false);
            bool c = state.dsChecked[i];
            if (ImGui::Checkbox(state.datasets[i].name.c_str(), &c)) {
                viewer.setCheckedDataset(i, c);
            }
        }
        ImGui::Separator();
        ImGui::SetNextItemWidth(-1); ImGui::InputFloat3("##pXYZ", state.partialXYZ.data());
        if (ImGui::Button("Compare", { -1, 0 })) viewer.updateHeatmaps();
    } else {
        ImGui::TextColored(COL_TEXT, "Dataset:");
        for (int i = 0; i < (int)state.datasets.size(); ++i) {
            bool sel = (i == state.activeDataset);
            if (ImGui::RadioButton(state.datasets[i].name.c_str(), sel))
                if (!sel) viewer.switchDataset(i);
        }
    }

    ImGui::Separator();
    if (state.isMode<NormalState>() || state.isMode<TotalCompState>()) {
        ImGui::TextColored(COL_ORANGE, "Cell A:");
        ImGui::SetNextItemWidth(-1); ImGui::InputFloat3("##cA", state.cellAXYZ.data(), "%.3f");
        if (ImGui::Button("Go##A", { -1, 0 })) viewer.goToCellA();
        ImGui::TextColored(COL_PURPLE, "Cell B:");
        ImGui::SetNextItemWidth(-1); ImGui::InputFloat3("##cB", state.cellBXYZ.data(), "%.3f");
        if (ImGui::Button("Go##B", { -1, 0 })) viewer.goToCellB();

        bool oneCellSelected = state.isMode<NormalState>()
                            && state.getSelectedCellA() >= 0
                            && state.getSelectedCellB() < 0;
        if (!oneCellSelected) ImGui::BeginDisabled();
        if (ImGui::Button("Export Spherical Camera", { -1, 0 })) viewer.exportSphericalCamera();
        if (!oneCellSelected) ImGui::EndDisabled();

        ImGui::Separator();
    }

    ImGui::TextColored(COL_GREEN, "Filter:");
    ImGui::SetNextItemWidth(-1); ImGui::InputFloat3("Min##f", state.flags.filterMin.data());
    ImGui::SetNextItemWidth(-1); ImGui::InputFloat3("Max##f", state.flags.filterMax.data());
    if (ImGui::Button("Apply", { -1, 0 })) viewer.applyFilter();
    ImGui::SameLine();
    if (ImGui::Button("Reset", { -1, 0 })) viewer.clearFilter();
    if (ImGui::Button("Scene Bounds", { -1, 0 })) viewer.setFilterToScene();
    if (state.flags.isFilterActive) {
        ImGui::TextColored(COL_GREEN, "%d / %d nodes",
            (int)state.filteredIndices.size(),
            (int)state.datasets[state.activeDataset].nodes.size());
    }
    ImGui::Separator();

    ImGui::Checkbox("Show all cells",  &state.flags.showAllWireframes);
    ImGui::Checkbox("Show filter box", &state.flags.showFilterBox);

    ImGui::Separator();
    if ((state.isMode<NormalState>() || state.isMode<TotalCompState>()) && !state.datasets.empty()) {
        auto& dataset = state.datasets[state.activeDataset];
        ImGui::TextColored(COL_TEXT, "%d nodes [%s]",
            (int)dataset.nodes.size(), dirKindToName(dataset.type));
        int selA = state.getSelectedCellA();
        int selB = state.getSelectedCellB();
        if (selA >= 0 && selA < (int)dataset.nodes.size()) {
            float tr  = viewer.filteredTotalRadiance(dataset);
            float pct = (tr > 0) ? (dataset.nodes[selA].meanRadiance / tr) * 100 : 0;
            ImGui::TextColored(COL_ORANGE, "A[%d]: %.2f%%", selA, pct);
        }
        if (selB >= 0 && selB < (int)dataset.nodes.size()) {
            float tr  = viewer.filteredTotalRadiance(dataset);
            float pct = (tr > 0) ? (dataset.nodes[selB].meanRadiance / tr) * 100 : 0;
            ImGui::TextColored(COL_PURPLE, "B[%d]: %.2f%%", selB, pct);
        }
    }
    ImGui::Separator();
    ImGui::TextColored({ 0.5f, 0.5f, 0.5f, 1 },
        "Click:A  Shift:B\nArrows:nav  Enter:KL\nEsc:clear  T:tiling\nS:cycle scene (comp)");
}
