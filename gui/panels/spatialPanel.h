#pragma once

class SDTreeViewer;

// Renders the spatial visualization. This picks
// call viewer.pickSpatialNode(). The camera input mutates the camera directly
// because it's a per-frame interaction.
//
// Caller is responsible for the surrounding ImGui::BeginChild/EndChild.
void renderSpatialPanel(SDTreeViewer& viewer);
