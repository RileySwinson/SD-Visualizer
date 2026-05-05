#include "sequenceBar.h"

#include "../../viewer/sdtreeViewer.h"
#include "../../globals/uiColors.h"

#include <imgui.h>

#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

using std::max;
using std::min;
using std::string;
using std::to_string;
using std::vector;

float renderSequenceBar(SDTreeViewer& viewer, int winW) {
    auto& state = viewer.state;

    bool anyGroups = false;
    for (auto& d : state.datasets) if (d.groupIndex >= 0) { anyGroups = true; break; }
    if (!anyGroups) return 0.f;

    constexpr float topH = 40.f;
    ImGui::SetNextWindowPos({ 0, 0 });
    ImGui::SetNextWindowSize({ (float)winW, topH });
    ImGui::Begin("##Top", 0,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground);

    int maxGroup = -1;
    for (auto& d : state.datasets) maxGroup = max(maxGroup, d.groupIndex);

    for (int g = 0; g <= maxGroup; ++g) {
        vector<int> members;
        for (int i = 0; i < (int)state.datasets.size(); ++i)
            if (state.datasets[i].groupIndex == g) members.push_back(i);
        if (members.empty()) continue;

        string gPrefix = state.datasets[members[0]].name;
        for (int mi = 1; mi < (int)members.size(); ++mi) {
            string& other = state.datasets[members[mi]].name;
            while (gPrefix.size() > 0 && other.substr(0, gPrefix.size()) != gPrefix) gPrefix.pop_back();
        }
        if (g > 0) ImGui::SameLine();
        ImGui::TextColored(COL_TEXT, "[%s", gPrefix.c_str()); ImGui::SameLine();

        for (int mi = 0; mi < (int)members.size(); ++mi) {
            int  dsIdx = members[mi];
            bool act   = (dsIdx == state.activeDataset);
            if (act) ImGui::PushStyleColor(ImGuiCol_Button,
                { COL_ORANGE.x, COL_ORANGE.y, COL_ORANGE.z, 0.7f });

            string suffix = state.datasets[dsIdx].name.substr(gPrefix.size());
            if (suffix.empty()) suffix = to_string(mi + 1);

            char btnId[64];
            std::snprintf(btnId, sizeof(btnId), "%s##g%d_%d", suffix.c_str(), g, mi);

            if (ImGui::Button(btnId, { 0, 24 })) {
                if (state.isShiftHeld) {
                    int before = min(state.activeDataset, dsIdx);
                    int after  = max(state.activeDataset, dsIdx);
                    if (before != after
                     && state.datasets[before].groupIndex >= 0
                     && state.datasets[before].groupIndex == state.datasets[after].groupIndex)
                    {
                        viewer.enterTotalDiff(before, after);
                    }
                } else {
                    viewer.switchDataset(dsIdx);
                }
            }
            if (act) ImGui::PopStyleColor();
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("%s", state.datasets[dsIdx].name.c_str());
                ImGui::EndTooltip();
            }
            ImGui::SameLine();
        }
        ImGui::TextColored(COL_TEXT, "]"); ImGui::SameLine();
    }

    for (int i = 0; i < (int)state.datasets.size(); ++i) {
        if (state.datasets[i].groupIndex >= 0) continue;
        bool act = (i == state.activeDataset);
        if (act) ImGui::PushStyleColor(ImGuiCol_Button,
            { COL_ORANGE.x, COL_ORANGE.y, COL_ORANGE.z, 0.7f });
        char btnId[64];
        std::snprintf(btnId, sizeof(btnId), "%s##s%d", state.datasets[i].name.c_str(), i);
        if (ImGui::Button(btnId, { 0, 24 })) viewer.switchDataset(i);
        if (act) ImGui::PopStyleColor();
        ImGui::SameLine();
    }

    if (auto* tc = state.asMode<TotalCompState>(); tc && tc->divResult.valid) {
        ImGui::TextColored(COL_TEXT,   "DIFF %d->%d ", tc->diffBefore + 1, tc->diffAfter + 1); ImGui::SameLine();
        ImGui::TextColored(COL_TEXT,   "U:%d", tc->divResult.numUnchanged);                    ImGui::SameLine();
        ImGui::TextColored(COL_ORANGE, "R:%d", tc->divResult.numRefined);                      ImGui::SameLine();
        ImGui::TextColored(COL_RED,    "D:%d", tc->divResult.numDivergent);                    ImGui::SameLine();
        if (ImGui::SmallButton("Exit Diff")) viewer.exitTotalDiff();
    }
    ImGui::End();
    return topH;
}
