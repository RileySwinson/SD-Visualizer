#include "spatialPanel.h"

#include "../../viewer/sdtreeViewer.h"

#include <imgui.h>

void renderSpatialPanel(SDTreeViewer& viewer) {
    auto&  state    = viewer.state;
    ImVec2 viewSize = ImGui::GetContentRegionAvail();
    ImVec2 viewPos  = ImGui::GetCursorScreenPos();

    viewer.render3D((int)viewSize.x, (int)viewSize.y);
    ImGui::Image(
        (ImTextureID)(intptr_t)viewer.fboColorTex.handle(),
        viewSize, { 0, 1 }, { 1, 0 }
    );

    if (ImGui::IsItemHovered()) {
        auto& io = ImGui::GetIO();

        // Camera input is a per-frame continuous interaction; mutate directly.
        if (io.MouseWheel != 0) viewer.cam.zoom(io.MouseWheel);
        if (ImGui::IsMouseDragging(0)) {
            auto d = io.MouseDelta;
            viewer.cam.orbit(-d.x * 0.005f, d.y * 0.005f);
        }
        if (ImGui::IsMouseDragging(1) || ImGui::IsMouseDragging(2)) {
            auto d = io.MouseDelta;
            viewer.cam.pan(-d.x, d.y);
        }

        // Discrete: a click that wasn't a drag picks a node.
        if (ImGui::IsMouseReleased(0)) {
            ImVec2 dd = ImGui::GetMouseDragDelta(0);
            if (dd.x * dd.x + dd.y * dd.y < 25) {
                ImVec2 mp     = io.MousePos;
                int    picked = viewer.pickNode(
                    mp.x - viewPos.x,
                    viewSize.y - (mp.y - viewPos.y),
                    viewSize.x, viewSize.y
                );
                if (picked >= 0) {
                    viewer.pickSpatialNode(picked, state.isShiftHeld);
                }
            }
        }
    }

    if (!state.datasets.empty()) {
        ImGui::GetWindowDrawList()->AddText(
            { viewPos.x + 10, viewPos.y + 5 },
            IM_COL32(249, 249, 250, 200),
            state.datasets[state.activeDataset].name.c_str()
        );
    }
}
