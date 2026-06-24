#include "directionalPanel.h"

#include "../../viewer/sdtreeViewer.h"
#include "../../globals/uiColors.h"
#include "../../domain/projection.h"
#include "../../domain/directional/parameterSpace.h"

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

using std::max;
using std::min;
using std::string;
using std::to_string;

void renderDirectionalPanel(SDTreeViewer& viewer) {
    auto& state = viewer.state;
    state.hover.active = false;

    if (state.numSlots == 0) {
        if (auto* it = state.asMode<IterationDiffState>(); it && (it->iterA < 0 || it->iterB < 0))
            ImGui::TextColored({ 0.5f, 0.5f, 0.5f, 1 },
                "Select two iterations (Click=A, Shift+Click=B)\nthen click a spatial node.");
        else
            ImGui::TextColored({ 0.5f, 0.5f, 0.5f, 1 }, "Select a node to view its PDF.");
        return;
    }

    bool klEligible = (state.isMode<PartialCompState>() || state.isMode<IterationDiffState>())
                   && state.numSlots >= 2;
    if (klEligible) {
        if (!state.kl.isActive) {
            if (ImGui::Button("Create KL Map")) viewer.beginKlPicking();
        } else {
            ImGui::TextColored(COL_CYAN, "Click two heatmaps to compare. A=%s B=%s",
                state.kl.slotA >= 0 ? to_string(state.kl.slotA + 1).c_str() : "?",
                state.kl.slotB >= 0 ? to_string(state.kl.slotB + 1).c_str() : "?");
            ImGui::SameLine();
            if (ImGui::SmallButton("Cancel")) viewer.cancelKlPicking();
        }
        ImGui::Separator();
    }

    ImVec2 avail = ImGui::GetContentRegionAvail();
    float  cbarW = 30, labelW = 70;
    float  slotH = avail.y / (float)max(state.numSlots, 1);
    float  hmSide = min(avail.x - cbarW - labelW - 20, slotH - 80);
    hmSide = max(hmSide, 60.f);

    for (int si = 0; si < state.numSlots; ++si) {
        auto& slot = state.slots[si];
        if (!slot.valid) continue;

        if (state.isMode<PartialCompState>() && si < MAX_COMP_SLOTS) {
            ImVec2 cp = ImGui::GetCursorScreenPos();
            auto&  cc = COMP_SLOT_COLORS[si];
            ImGui::GetWindowDrawList()->AddRectFilled({ cp.x, cp.y + 2 }, { cp.x + 12, cp.y + 14 },
                IM_COL32((int)(cc[0] * 255), (int)(cc[1] * 255), (int)(cc[2] * 255), 255));
            ImGui::Dummy({ 16, 0 }); ImGui::SameLine();
        }

        ImGui::TextColored(COL_CYAN, "%s", slot.typeLabel.c_str());

        if (slot.dirType == BTC && slot.numTilings > 0) {
            ImGui::SameLine();
            int ti = state.slotTiling[si];
            if (ti < 0) ImGui::PushStyleColor(ImGuiCol_Button,
                { COL_ORANGE.x, COL_ORANGE.y, COL_ORANGE.z, 0.7f });
            char avgLb[16]; std::snprintf(avgLb, 16, "Avg##ti%d", si);
            if (ImGui::SmallButton(avgLb)) viewer.setSlotTiling(si, -1);
            if (ti < 0) ImGui::PopStyleColor();
            for (int t = 0; t < slot.numTilings; ++t) {
                ImGui::SameLine();
                char lb[16]; std::snprintf(lb, 16, "%d##ti%d_%d", t + 1, si, t);
                if (t == ti) ImGui::PushStyleColor(ImGuiCol_Button,
                    { COL_ORANGE.x, COL_ORANGE.y, COL_ORANGE.z, 0.7f });
                if (ImGui::SmallButton(lb)) viewer.setSlotTiling(si, t);
                if (t == ti) ImGui::PopStyleColor();
            }
        }
        else if (slot.dirType == CostQuadTree) {
            // Cost-quadtree slots get a Radiance/Costs toggle.
            // Convention: slotTiling != 1 means radiance, slotTiling == 1 means costs.
            ImGui::SameLine();
            int  ti           = state.slotTiling[si];
            bool showingRad   = (ti != 1);
            bool showingCosts = (ti == 1);

            if (showingRad) ImGui::PushStyleColor(ImGuiCol_Button,
                { COL_ORANGE.x, COL_ORANGE.y, COL_ORANGE.z, 0.7f });
            char radLb[24]; std::snprintf(radLb, 24, "Radiance##ti%d", si);
            if (ImGui::SmallButton(radLb)) viewer.setSlotTiling(si, 0);
            if (showingRad) ImGui::PopStyleColor();

            ImGui::SameLine();
            if (showingCosts) ImGui::PushStyleColor(ImGuiCol_Button,
                { COL_ORANGE.x, COL_ORANGE.y, COL_ORANGE.z, 0.7f });
            char costLb[24]; std::snprintf(costLb, 24, "Costs##ti%d", si);
            if (ImGui::SmallButton(costLb)) viewer.setSlotTiling(si, 1);
            if (showingCosts) ImGui::PopStyleColor();
        }

        if (auto* it = state.asMode<IterationDiffState>(); it && si == 1
         && !it->showAll && (int)it->childrenIndices.size() > 1)
        {
            ImGui::SameLine(); ImGui::TextColored(COL_PURPLE, " Child:");
            for (int ci = 0; ci < (int)it->childrenIndices.size(); ++ci) {
                ImGui::SameLine();
                char childLb[16]; std::snprintf(childLb, 16, "%d##sc%d", ci + 1, ci);
                bool wasActive = (ci == it->activeChild);
                if (wasActive) ImGui::PushStyleColor(ImGuiCol_Button,
                    { COL_PURPLE.x, COL_PURPLE.y, COL_PURPLE.z, 0.7f });
                if (ImGui::SmallButton(childLb)) viewer.setActiveChild(ci);
                if (wasActive) ImGui::PopStyleColor();
            }
        }

        float exportBtnW = ImGui::CalcTextSize("Export Heatmap").x
                         + ImGui::GetStyle().FramePadding.x * 2;
        ImGui::SameLine(ImGui::GetContentRegionMax().x - exportBtnW);
        char exportId[32]; std::snprintf(exportId, 32, "Export Heatmap##e%d", si);
        if (ImGui::SmallButton(exportId)) viewer.exportSlotHeatmap(si);

        ImGui::TextWrapped("%s", slot.title.c_str());

        // The active parameter space sets the image aspect (2:1 for spherical).
        float aspect = (slot.gridH > 0) ? (float)slot.gridW / (float)slot.gridH : 1.0f;
        float drawH  = hmSide;
        float drawW  = hmSide * aspect;
        float maxW   = max(avail.x - cbarW - labelW - 20, 60.f);
        if (drawW > maxW) { drawW = maxW; drawH = drawW / aspect; }

        ImVec2 imgPos = ImGui::GetCursorScreenPos();
        ImGui::Image((ImTextureID)(intptr_t)viewer.slotTex[si].handle(),
                     { drawW, drawH }, { 0, 1 }, { 1, 0 });

        if (state.insp.valid && state.insp.slotIndex == si
         && state.flags.paramSpace != ParameterSpace::Spherical
         && !(slot.dirType == BTC && state.slotTiling[si] < 0))
        {
            ImVec2 dMin = { imgPos.x + state.insp.bounds.minA * drawW,
                            imgPos.y + (1 - state.insp.bounds.maxB) * drawH };
            ImVec2 dMax = { imgPos.x + state.insp.bounds.maxA * drawW,
                            imgPos.y + (1 - state.insp.bounds.minB) * drawH };
            ImGui::GetWindowDrawList()->AddRect(dMin, dMax,
                IM_COL32(255, 255, 255, 255), 0, 0, 2);
        }

        if (state.kl.isActive && (si == state.kl.slotA || si == state.kl.slotB)) {
            ImGui::GetWindowDrawList()->AddRect(imgPos,
                { imgPos.x + drawW, imgPos.y + drawH },
                IM_COL32(0, 221, 255, 255), 0, 0, 3);
        }

        // Hover ray: continuous per-frame; mutate directly.
        float hu, hv;
        if (ImGui::IsItemHovered() && viewer.heatmapUV(imgPos, { drawW, drawH }, hu, hv)) {
            state.hover.active    = true;
            state.hover.origin    = slot.nodePos;
            state.hover.direction = parameterSpacePixelToDirection(state.flags.paramSpace, hu, hv);
            ImGui::BeginTooltip();
            ImGui::Text("UV:(%.3f,%.3f)", hu, hv);
            ImGui::EndTooltip();
        }

        // Click: KL pick OR bin inspect, depending on KL-picking mode.
        if (ImGui::IsItemClicked(0)) {
            if (state.kl.isActive) {
                viewer.pickKlSlot(si);
            } else {
                bool canInspect = !(slot.dirType == BTC && state.slotTiling[si] < 0);
                float cu, cv;
                if (canInspect && viewer.heatmapUV(imgPos, { drawW, drawH }, cu, cv)) {
                    viewer.inspectBin(si, cu, cv);
                }
            }
        }

        ImGui::SameLine();
        ImGui::Image((ImTextureID)(intptr_t)viewer.slotCbar[si].handle(),
                     { cbarW, drawH }, { 0, 1 }, { 1, 0 });
        ImGui::SameLine();
        ImGui::BeginGroup();

        char lb[32];
        std::snprintf(lb, 32, "%.1e", slot.vMax); ImGui::Text("%s", lb);
        ImGui::Dummy({ 0, drawH / 2 - 20 });
        std::snprintf(lb, 32, "%.1e", std::sqrt(slot.vMin * slot.vMax)); ImGui::Text("%s", lb);
        ImGui::Dummy({ 0, drawH / 2 - 20 });
        std::snprintf(lb, 32, "%.1e", slot.vMin); ImGui::Text("%s", lb);
        ImGui::EndGroup();

        bool isCostsView = (slot.dirType == CostQuadTree && state.slotTiling[si] == 1);

        if (!slot.useKL && !isCostsView) {
            if (slot.isIntegralValid) ImGui::Text("Integral: %.4f", slot.integral);
            else ImGui::TextColored(COL_RED, "!! Integral: %.4f (expected ~1.0)", slot.integral);
        }

        if (state.insp.valid && state.insp.slotIndex == si) {
            ImGui::TextColored(COL_TEXT, "Bin: %.4f%% of node %s",
                state.insp.pct, isCostsView ? "cost" : "radiance");
            ImGui::Text("  UV[%.3f,%.3f]-[%.3f,%.3f]  Area:%.6f",
                state.insp.bounds.minA, state.insp.bounds.minB,
                state.insp.bounds.maxA, state.insp.bounds.maxB, state.insp.area);
        }
        if (si < state.numSlots - 1) ImGui::Separator();
    }
}
