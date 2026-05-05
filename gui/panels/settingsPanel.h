#pragma once

class SDTreeViewer;

// Renders the left settings panel. Discrete actions call viewer methods
// directly; continuous bindings (XYZ floats, filter min/max, display flag
// checkboxes) are mutated through ImGui pointer bindings directly.
//
// Caller is responsible for the surrounding ImGui::BeginChild/EndChild.
void renderSettingsPanel(SDTreeViewer& viewer);
