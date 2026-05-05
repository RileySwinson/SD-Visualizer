#pragma once

#include "../globals/config.h"
#include "../domain/state/appState.h"
#include "../gui/glResources.h"
#include "../gui/orbitCamera.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <Eigen/Dense>

#include <array>
#include <memory>
#include <string>
#include <vector>

// Top-level viewer. Owns AppState (the source of truth for runtime state),
// GL handles, the orbit camera, and the per-frame ImGui driver. Panels and
// input handlers call methods here directly to mutate state; mode-conditional
// behaviour lives behind std::visit on state.mode.
class SDTreeViewer {
public:
    AppState state;

    GlProgram pointProg;
    GlProgram lineProg;
    GlVertexArray pointVAO;
    GlVertexArray lineVAO;
    GlBuffer pointVBO;
    GlBuffer lineVBO;
    GlFramebuffer fbo;
    GlTexture fboColorTex;
    GlRenderbuffer fboDepthRB;
    int fboW = 0, fboH = 0;
    GlTexture slotTex[MAX_COMP_SLOTS];
    GlTexture slotCbar[MAX_COMP_SLOTS];

    OrbitCamera cam;
    int pointCount = 0;
    Eigen::Vector3f sceneMin, sceneMax;

    void init();
    void resizeFBO(int w, int h);

    // ---- mode / dataset ----
    void switchDataset(int i);
    void enterNormalMode();
    void enterPartialMode();
    void enterIterationMode(int groupIdx);
    void enterTotalDiff(int before, int after);
    void exitTotalDiff();

    // ---- sequence iteration ----
    void setSequenceGroup(int groupIdx);
    void clickIterationButton(int dsIdx, bool shift);
    void toggleShowAllChildren(bool value);
    void setActiveChild(int childIdx);

    // ---- selection ----
    void pickSpatialNode(int idx, bool shift);
    void goToCellA();
    void goToCellB();
    void clearSelection();
    void activateKL();
    void navNode(int delta);

    // ---- filter ----
    void recomputeFilter();
    void applyFilter();
    void clearFilter();
    void setFilterToScene();

    // ---- slot operations ----
    void setSlotTiling(int slotIdx, int tilingIdx);
    void cycleBtcTiling();
    void beginKlPicking();
    void cancelKlPicking();
    void pickKlSlot(int slotIdx);
    void inspectBin(int slotIdx, float u, float v);

    // ---- partial compare ----
    void setCheckedDataset(int idx, bool value);
    void cycleScene();

    // ---- queries / renderer plumbing ----
    const std::vector<int>& activeIndex() const;
    float filteredTotalRadiance(const SDTree& dataset) const;
    void  cacheSelectedXYZ();
    void  updateSequenceChildren();
    void  rebuildPointCloud();
    void  buildSlotTexture(int slotIndex, const std::shared_ptr<IDirectional>& dtree);
    void  buildKL(
        int slotIndex,
        const std::vector<DirLeaf>& leavesA,
        const std::vector<DirLeaf>& leavesB
    );
    void  updateHeatmaps();
    void  rebuildSlot(int slotIndex);
    void  render3D(int w, int h);
    int   pickNode(float screenX, float screenY, float viewWidth, float viewHeight);
    bool  heatmapUV(ImVec2 imgPos, ImVec2 imgSize, float& u, float& v);
    std::vector<int> getSequenceGroups() const;
    std::vector<int> getGroupMembers(int groupIdx) const;
    void  exportSlotHeatmap(int slotIndex);

    // Per-frame entry points.
    void renderUI(GLFWwindow* win, int winW, int winH);
    void processKeys(GLFWwindow* win);
};

std::string commonPrefix(const std::vector<SDTree>& datasets);
