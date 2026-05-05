#pragma once

class SDTreeViewer;

// Renders the top bar listing dataset buttons, with bracketed-sequence groups
// shown inline. Click switches dataset; shift-click within a group enters
// TotalComp (FULL_DIFF) mode.
//
// Returns the height consumed (0 if there are no groups to display).
float renderSequenceBar(SDTreeViewer& viewer, int winW);
