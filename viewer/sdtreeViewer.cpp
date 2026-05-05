#include "sdtreeViewer.h"

#include "../globals/uiColors.h"
#include "../globals/palette.h"
#include "../domain/projection.h"
#include "../domain/compute/filter.h"
#include "../domain/compute/nodeQueries.h"
#include "../domain/compute/rasterize.h"
#include "../domain/compute/pointCloudBuild.h"
#include "../domain/compute/structureCompare.h"
#include "../gui/geometryRenderer.h"
#include "../gui/panels/sequenceBar.h"
#include "../gui/panels/settingsPanel.h"
#include "../gui/panels/spatialPanel.h"
#include "../gui/panels/directionalPanel.h"
#include "../gui/pngExport.h"
#include "../gui/shaders.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <numeric>
#include <utility>
#include <variant>

using std::array;
using std::find;
using std::max;
using std::min;
using std::shared_ptr;
using std::string;
using std::swap;
using std::to_string;
using std::vector;

string commonPrefix(const vector<SDTree>& datasets) {
    if (datasets.size() < 2) return "";

    string prefix = datasets[0].name;
    for (size_t i = 1; i < datasets.size(); ++i) {
        while (prefix.size() > 0 && datasets[i].name.substr(0, prefix.size()) != prefix) {
            prefix.pop_back();
        }
    }
    return prefix;
}

static Eigen::AlignedBox3f filterBoxOf(const AppState& s) {
    return Eigen::AlignedBox3f(s.flags.filterMin, s.flags.filterMax);
}

void SDTreeViewer::init() {
    pointProg = GlProgram(linkProgram(POINT_VS, POINT_FS));
    lineProg  = GlProgram(linkProgram(LINE_VS,  LINE_FS));
    pointVAO.create(); pointVBO.create();
    lineVAO.create();  lineVBO.create();

    for (int i = 0; i < MAX_COMP_SLOTS; ++i) {
        slotTex[i].create();  glBindTexture(GL_TEXTURE_2D, slotTex[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        slotCbar[i].create(); glBindTexture(GL_TEXTURE_2D, slotCbar[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        state.slotTiling[i] = -1;
    }

    fbo.create();
    fboColorTex.create();
    fboDepthRB.create();
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_DEPTH_TEST);

    state.dsChecked.resize(state.datasets.size(), false);
    if (!state.dsChecked.empty()) state.dsChecked[0] = true;
    state.prefix = commonPrefix(state.datasets);
    if (!state.datasets.empty()) switchDataset(0);
}

void SDTreeViewer::resizeFBO(int w, int h) {
    if (w == fboW && h == fboH) return;
    fboW = max(w, 1); fboH = max(h, 1);

    glBindTexture(GL_TEXTURE_2D, fboColorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, fboW, fboH, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindRenderbuffer(GL_RENDERBUFFER, fboDepthRB);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, fboW, fboH);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fboColorTex, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, fboDepthRB);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SDTreeViewer::switchDataset(int i) {
    if (i < 0 || i >= (int)state.datasets.size()) return;

    state.activeDataset = i;
    if (auto* n = state.asMode<NormalState>()) n->isKlMode = false;
    state.insp.valid    = false;
    state.hover.active  = false;

    // Switching dataset exits diff/sequence modes (semantics from legacy).
    if (state.isMode<TotalCompState>() || state.isMode<IterationDiffState>()) {
        state.mode = NormalState{};
    }
    if (state.flags.isFilterActive) recomputeFilter();

    int newA = findNode(state.datasets[state.activeDataset], state.cellAXYZ);
    state.setSelectedCellAIndex(newA);

    int curB = state.getSelectedCellB();
    if (curB >= 0) {
        if (auto* n = state.asMode<NormalState>()) {
            n->selectedCellB = findNode(state.datasets[state.activeDataset], state.cellBXYZ);
        }
    }

    rebuildPointCloud();
    cacheSelectedXYZ();
    updateHeatmaps();
}

void SDTreeViewer::recomputeFilter() {
    state.filteredIndices = computeFilteredIndices(state.datasets[state.activeDataset], filterBoxOf(state));
    if (!state.filteredIndices.empty()) {
        state.setSelectedCellAIndex(state.filteredIndices[0]);
    }
}

void SDTreeViewer::applyFilter() {
    state.flags.isFilterActive = true;
    recomputeFilter();

    if (auto* n = state.asMode<NormalState>()) {
        n->selectedCellB = -1;
        n->isKlMode      = false;
    }
    
    rebuildPointCloud();
    cacheSelectedXYZ();
    updateHeatmaps();
}

void SDTreeViewer::clearFilter() {
    state.flags.isFilterActive = false;
    rebuildPointCloud();
    updateHeatmaps();
}

const vector<int>& SDTreeViewer::activeIndex() const {
    static vector<int> result;
    result.clear();

    auto& dataset = state.datasets[state.activeDataset];
    if (state.flags.isFilterActive) {
        result = state.filteredIndices;
    } else {
        result.resize(dataset.nodes.size());
        std::iota(result.begin(), result.end(), 0);
    }

    if (auto* tc = state.asMode<TotalCompState>(); tc && tc->divResult.valid) {
        vector<int> diffOnly;
        for (int i : result) {
            if (i < (int)tc->divResult.status.size() && tc->divResult.status[i] != DIV_UNCHANGED) {
                diffOnly.push_back(i);
            }
        }
        result = diffOnly;
    }
    return result;
}

float SDTreeViewer::filteredTotalRadiance(const SDTree& dataset) const {
    return ::filteredTotalRadiance(dataset, filterBoxOf(state), state.flags.isFilterActive);
}

void SDTreeViewer::cacheSelectedXYZ() {
    auto& dataset = state.datasets[state.activeDataset];

    int idxA = state.getSelectedCellA();
    if (idxA >= 0 && idxA < (int)dataset.nodes.size()) {
        state.cellAXYZ = dataset.nodes[idxA].pos;
    }

    int idxB = state.getSelectedCellB();
    if (idxB >= 0 && idxB < (int)dataset.nodes.size()) {
        state.cellBXYZ = dataset.nodes[idxB].pos;
    }
}

void SDTreeViewer::updateSequenceChildren() {
    auto* it = state.asMode<IterationDiffState>();
    if (!it) return;

    it->childrenIndices.clear();
    it->activeChild = 0;

    if (it->iterA < 0 || it->iterB < 0) return;
    if (it->iterA >= (int)state.datasets.size() || it->iterB >= (int)state.datasets.size()) return;
    if (it->parentCellIdx < 0 || it->parentCellIdx >= (int)state.datasets[it->iterA].nodes.size()) return;

    auto& parent = state.datasets[it->iterA].nodes[it->parentCellIdx];
    it->childrenIndices = findDescendants(state.datasets[it->iterB], parent.pos, parent.size);

    if ((int)it->childrenIndices.size() > IterationDiffState::MAX_SHOW_ALL) {
        it->showAll = false;
    }
}

void SDTreeViewer::rebuildPointCloud() {
    auto& dataset = state.datasets[state.activeDataset];
    if (dataset.nodes.empty()) return;

    auto& idx = activeIndex();

    const vector<DivStatus>* divPtr = nullptr;
    if (auto* tc = state.asMode<TotalCompState>(); tc && tc->divResult.valid) {
        divPtr = &tc->divResult.status;
    }

    PointCloudResult pc = buildPointCloud(dataset, idx, divPtr);
    sceneMin = pc.sceneMin;
    sceneMax = pc.sceneMax;

    pointCount = (int)pc.vertices.size();
    glBindVertexArray(pointVAO);
    glBindBuffer(GL_ARRAY_BUFFER, pointVBO);
    glBufferData(GL_ARRAY_BUFFER, pc.vertices.size() * sizeof(PointVertex), pc.vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(PointVertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(PointVertex), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    cam.target   = (sceneMin + sceneMax) * 0.5f;
    cam.distance = (sceneMax - sceneMin).norm() * 1.2f;
}

void SDTreeViewer::buildSlotTexture(int slotIndex, const shared_ptr<IDirectional>& dtree) {
    static float   grid[HMAP_RES][HMAP_RES];
    static uint8_t pixels[HMAP_RES * HMAP_RES * 4];
    static uint8_t cbarPixels[256 * 4];

    auto& slot       = state.slots[slotIndex];
    int   tilingIdx  = state.slotTiling[slotIndex];

    if (dtree) {
        dtree->fillGrid(grid, tilingIdx);
    } else {
        for (int y = 0; y < HMAP_RES; ++y) {
            for (int x = 0; x < HMAP_RES; ++x) {
                grid[y][x] = PDF_FLOOR;
            }
        }
    }

    // The integral check (PDF ~= 1 over the sphere) only applies when the
    // grid we just rasterised is a probability density. Cost view holds raw
    // per-leaf scalars, so we skip the check there.
    bool isCostsView = (dtree && dtree->kind() == CostQuadTree && tilingIdx == 1);

    float pdfIntegral = 0;
    if (dtree && !isCostsView) {
        for (int y = 0; y < HMAP_RES; ++y) {
            for (int x = 0; x < HMAP_RES; ++x) {
                float u = ((float)x + 0.5f) / HMAP_RES;
                float v = ((float)y + 0.5f) / HMAP_RES;
                float val = dtree->evalPDF(u, v, tilingIdx);
                if (dtree->isBTC()) val *= EquirectToEqualArea::jacobian(v);
                pdfIntegral += val;
            }
        }

        pdfIntegral /= (HMAP_RES * HMAP_RES);
    }

    slot.integral        = pdfIntegral;
    slot.isIntegralValid = isCostsView || (std::fabs(pdfIntegral - 1.0f) <= 0.05f) || !dtree;

    float vMin = 1e30f, vMax = -1e30f;
    for (int y = 0; y < HMAP_RES; ++y) {
        for (int x = 0; x < HMAP_RES; ++x) {
            vMin = min(vMin, grid[y][x]);
            vMax = max(vMax, grid[y][x]);
        }
    }

    if (vMin >= vMax) vMax = vMin * 10;
    slot.vMin = vMin;
    slot.vMax = vMax;

    auto& colormap = slot.useKL ? CMAP_KL : CMAP_PDF;

    buildHeatmapPixels(grid, colormap, pixels, slot.vMin, slot.vMax);
    glBindTexture(GL_TEXTURE_2D, slotTex[slotIndex]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, HMAP_RES, HMAP_RES, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    buildColorbarPixels(colormap, cbarPixels, 256);
    glBindTexture(GL_TEXTURE_2D, slotCbar[slotIndex]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, cbarPixels);
}

void SDTreeViewer::buildKL(int slotIndex, const vector<DirLeaf>& leavesA, const vector<DirLeaf>& leavesB)
{
    static float   gridA[HMAP_RES][HMAP_RES];
    static float   gridB[HMAP_RES][HMAP_RES];
    static float   gridKL[HMAP_RES][HMAP_RES];
    static uint8_t pixels[HMAP_RES * HMAP_RES * 4];
    static uint8_t cbarPixels[256 * 4];

    rasterizeLeavesPDF(leavesA, gridA);
    rasterizeLeavesPDF(leavesB, gridB);

    float klMax   = 1e-6f;
    float klTotal = 0;

    for (int y = 0; y < HMAP_RES; ++y) {
        for (int x = 0; x < HMAP_RES; ++x) {
            float a  = gridA[y][x];
            float b  = gridB[y][x];
            float kl = a * std::log(a / b) + b * std::log(b / a);
            if (!std::isfinite(kl) || kl < 1e-6f) kl = 1e-6f;
            gridKL[y][x] = kl;
            klMax  = max(klMax, kl);
            klTotal += kl;
        }
    }
    klTotal /= (float)(HMAP_RES * HMAP_RES);

    auto& slot = state.slots[slotIndex];
    slot.vMin  = 1e-6f;
    slot.vMax  = (klMax <= slot.vMin * 10) ? slot.vMin * 10 : klMax;
    slot.useKL = true;

    buildHeatmapPixels(gridKL, CMAP_KL, pixels, slot.vMin, slot.vMax);
    glBindTexture(GL_TEXTURE_2D, slotTex[slotIndex]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, HMAP_RES, HMAP_RES, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    buildColorbarPixels(CMAP_KL, cbarPixels, 256);
    glBindTexture(GL_TEXTURE_2D, slotCbar[slotIndex]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, cbarPixels);

    char buffer[256];
    std::snprintf(buffer, 256, "KL Divergence  D_Sym: %.4f", klTotal);
    slot.title           = buffer;
    slot.barLabel        = "Divergence";
    slot.typeLabel       = "KL Map";
    slot.valid           = true;
    slot.integral        = klTotal;
    slot.isIntegralValid = true;
}

void SDTreeViewer::navNode(int delta) {
    if (auto* iter = state.asMode<IterationDiffState>();
        iter && !state.isShiftHeld && !iter->showAll && !iter->childrenIndices.empty())
    {
        int n = (int)iter->childrenIndices.size();
        iter->activeChild = ((iter->activeChild + delta) % n + n) % n;
        updateHeatmaps();
        return;
    }

    auto& idx = activeIndex();
    if (idx.empty()) return;

    auto* normal = state.asMode<NormalState>();
    bool  navB   = state.isShiftHeld && normal && normal->selectedCellB >= 0;

    if (navB) {
        auto it  = find(idx.begin(), idx.end(), normal->selectedCellB);
        int  pos = (it != idx.end()) ? (int)(it - idx.begin()) : 0;
        pos = (pos + delta + (int)idx.size()) % (int)idx.size();
        normal->selectedCellB = idx[pos];
    } else {
        int curA = state.getSelectedCellA();
        auto it  = find(idx.begin(), idx.end(), curA);
        int  pos = (it != idx.end()) ? (int)(it - idx.begin()) : 0;
        pos = (pos + delta + (int)idx.size()) % (int)idx.size();
        state.setSelectedCellAIndex(idx[pos]);

        if (!state.isShiftHeld && normal) {
            normal->selectedCellB = -1;
            normal->isKlMode      = false;
        }
    }

    cacheSelectedXYZ();
    if (state.isMode<IterationDiffState>()) updateSequenceChildren();
    updateHeatmaps();
}

void SDTreeViewer::updateHeatmaps() {
    state.insp.valid    = false;
    state.hover.active  = false;
    state.numSlots      = 0;
    state.kl            = {};

    if (auto* it = state.asMode<IterationDiffState>()) {
        if (it->iterA < 0 || it->iterB < 0) return;
        if (it->iterA >= (int)state.datasets.size() || it->iterB >= (int)state.datasets.size()) return;

        auto& parentDS = state.datasets[it->iterA];
        if (it->parentCellIdx < 0 || it->parentCellIdx >= (int)parentDS.nodes.size()) return;

        auto& parentNode = parentDS.nodes[it->parentCellIdx];

        {
            auto& slot = state.slots[0];
            slot.leaves.clear();
            parentNode.dTree->collectLeaves(slot.leaves);
            slot.totalRad     = filteredTotalRadiance(parentDS);
            slot.nodePos      = parentNode.pos;
            slot.datasetIndex = it->iterA;
            slot.nodeIndex    = it->parentCellIdx;
            slot.useKL        = false;
            slot.dirType      = parentDS.type;
            slot.numTilings   = parentNode.dTree->getNumDistributions();
            state.slotTiling[0] = -1;

            float pct = (slot.totalRad > 0) ? (parentNode.meanRadiance / slot.totalRad) * 100 : 0;
            char buf[256];
            std::snprintf(buf, 256, "Parent [%s] Node %d (%.2f%%)",
                          parentDS.name.c_str(), it->parentCellIdx, pct);
            slot.title     = buf;
            slot.barLabel  = "Radiance";
            slot.typeLabel = dirKindToName(slot.dirType);
            slot.valid     = true;
            buildSlotTexture(0, parentNode.dTree);
            state.numSlots = 1;
        }

        if (it->childrenIndices.empty()) return;

        auto& newerDS = state.datasets[it->iterB];

        if (it->showAll && (int)it->childrenIndices.size() <= IterationDiffState::MAX_SHOW_ALL) {
            for (int ci = 0; ci < (int)it->childrenIndices.size() && state.numSlots < MAX_COMP_SLOTS; ++ci) {
                int childIdx = it->childrenIndices[ci];
                if (childIdx < 0 || childIdx >= (int)newerDS.nodes.size()) continue;
                auto& childNode = newerDS.nodes[childIdx];

                auto& slot = state.slots[state.numSlots];
                slot.leaves.clear();
                childNode.dTree->collectLeaves(slot.leaves);
                slot.totalRad     = filteredTotalRadiance(newerDS);
                slot.nodePos      = childNode.pos;
                slot.datasetIndex = it->iterB;
                slot.nodeIndex    = childIdx;
                slot.useKL        = false;
                slot.dirType      = newerDS.type;
                slot.numTilings   = childNode.dTree->getNumDistributions();
                state.slotTiling[state.numSlots] = -1;

                float pct = (slot.totalRad > 0) ? (childNode.meanRadiance / slot.totalRad) * 100 : 0;
                char buf[256];
                std::snprintf(buf, 256, "Child %d/%d [%s] Node %d (%.2f%%)",
                              ci + 1, (int)it->childrenIndices.size(),
                              newerDS.name.c_str(), childIdx, pct);
                slot.title     = buf;
                slot.barLabel  = "Radiance";
                slot.typeLabel = dirKindToName(slot.dirType);
                slot.valid     = true;
                buildSlotTexture(state.numSlots, childNode.dTree);
                state.numSlots++;
            }
        } else {
            if (it->activeChild >= (int)it->childrenIndices.size()) it->activeChild = 0;
            int childIdx = it->childrenIndices[it->activeChild];
            if (childIdx >= 0 && childIdx < (int)newerDS.nodes.size()) {
                auto& childNode = newerDS.nodes[childIdx];

                auto& slot = state.slots[1];
                slot.leaves.clear();
                childNode.dTree->collectLeaves(slot.leaves);
                slot.totalRad     = filteredTotalRadiance(newerDS);
                slot.nodePos      = childNode.pos;
                slot.datasetIndex = it->iterB;
                slot.nodeIndex    = childIdx;
                slot.useKL        = false;
                slot.dirType      = newerDS.type;
                slot.numTilings   = childNode.dTree->getNumDistributions();
                state.slotTiling[1] = -1;

                float pct = (slot.totalRad > 0) ? (childNode.meanRadiance / slot.totalRad) * 100 : 0;
                char buf[256];
                std::snprintf(buf, 256, "Child %d/%d [%s] Node %d (%.2f%%)",
                              it->activeChild + 1, (int)it->childrenIndices.size(),
                              newerDS.name.c_str(), childIdx, pct);
                slot.title     = buf;
                slot.barLabel  = "Radiance";
                slot.typeLabel = dirKindToName(slot.dirType);
                slot.valid     = true;
                buildSlotTexture(1, childNode.dTree);
                state.numSlots = 2;
            }
        }
        return;
    }

    if (state.isMode<PartialCompState>()) {
        for (int d = 0; d < (int)state.datasets.size() && state.numSlots < MAX_COMP_SLOTS; ++d) {
            if (d >= (int)state.dsChecked.size() || !state.dsChecked[d]) continue;

            int nodeIdx = findNode(state.datasets[d], state.partialXYZ);

            auto& slot = state.slots[state.numSlots];
            slot.leaves.clear();
            state.datasets[d].nodes[nodeIdx].dTree->collectLeaves(slot.leaves);

            float total       = filteredTotalRadiance(state.datasets[d]);
            slot.totalRad     = total;
            slot.nodePos      = state.datasets[d].nodes[nodeIdx].pos;
            slot.datasetIndex = d;
            slot.nodeIndex    = nodeIdx;
            slot.useKL        = false;
            slot.dirType      = state.datasets[d].type;
            slot.numTilings   = state.datasets[d].nodes[nodeIdx].dTree->getNumDistributions();
            state.slotTiling[state.numSlots] = -1;

            float pct = (total > 0) ? (state.datasets[d].nodes[nodeIdx].meanRadiance / total) * 100 : 0;
            char buf[256];
            std::snprintf(buf, 256, "%s  Node %d  (%.2f%%)",
                          state.datasets[d].name.c_str(), nodeIdx, pct);
            slot.title     = buf;
            slot.barLabel  = "Radiance";
            slot.typeLabel = dirKindToName(slot.dirType);
            slot.valid     = true;

            buildSlotTexture(state.numSlots, state.datasets[d].nodes[nodeIdx].dTree);
            state.numSlots++;
        }
        return;
    }

    auto& dataset = state.datasets[state.activeDataset];
    int   selA    = state.getSelectedCellA();
    if (dataset.nodes.empty() || selA < 0 || selA >= (int)dataset.nodes.size()) return;

    auto* normal = state.asMode<NormalState>();
    if (normal && normal->isKlMode && normal->selectedCellB >= 0
     && normal->selectedCellB < (int)dataset.nodes.size())
    {
        vector<DirLeaf> leavesA, leavesB;
        dataset.nodes[selA].dTree->collectLeaves(leavesA);
        dataset.nodes[normal->selectedCellB].dTree->collectLeaves(leavesB);

        auto& slot = state.slots[0];
        slot.nodePos      = dataset.nodes[selA].pos;
        slot.datasetIndex = state.activeDataset;
        slot.nodeIndex    = selA;
        buildKL(0, leavesA, leavesB);

        char buf[256];
        std::snprintf(buf, 256, "KL  Node %d vs %d", selA, normal->selectedCellB);
        slot.title    = buf;
        state.numSlots = 1;
        return;
    }

    auto& slot = state.slots[0];
    slot.leaves.clear();
    dataset.nodes[selA].dTree->collectLeaves(slot.leaves);

    float total = filteredTotalRadiance(dataset);
    float pct   = (total > 0) ? (dataset.nodes[selA].meanRadiance / total) * 100 : 0;

    slot.totalRad     = total;
    slot.nodePos      = dataset.nodes[selA].pos;
    slot.datasetIndex = state.activeDataset;
    slot.nodeIndex    = selA;
    slot.useKL        = false;
    slot.dirType      = dataset.type;
    slot.numTilings   = dataset.nodes[selA].dTree->getNumDistributions();
    state.slotTiling[0] = -1;

    char buf[256];
    std::snprintf(buf, 256, "Node %d  (%.2f%% of %s radiance)",
                  selA, pct, state.flags.isFilterActive ? "filtered" : "scene");
    slot.title     = buf;
    slot.barLabel  = "Radiance";
    slot.typeLabel = dirKindToName(slot.dirType);
    slot.valid     = true;

    buildSlotTexture(0, dataset.nodes[selA].dTree);
    state.numSlots = 1;
}

void SDTreeViewer::render3D(int w, int h) {
    resizeFBO(w, h);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, fboW, fboH);
    glClearColor(COL_BG.x, COL_BG.y, COL_BG.z, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float aspect = (float)fboW / max(fboH, 1);
    Eigen::Matrix4f proj = OrbitCamera::perspective(0.8f, aspect, 0.01f, 1000.f);
    Eigen::Matrix4f mvp  = proj * cam.viewMatrix();

    glUseProgram(pointProg);
    glUniformMatrix4fv(glGetUniformLocation(pointProg, "uMVP"), 1, GL_FALSE, mvp.data());
    glUniform1f(glGetUniformLocation(pointProg, "uPointSize"), 4.0f);
    glUniform1f(glGetUniformLocation(pointProg, "uRefDist"),  (sceneMax - sceneMin).norm());
    glBindVertexArray(pointVAO);
    glDrawArrays(GL_POINTS, 0, pointCount);

    auto& dataset = state.datasets[state.activeDataset];

    auto box = [&](
        const Eigen::Vector3f& center, const Eigen::Vector3f& size,
        array<float, 4> color, float lineWidth
    ) {
        drawBox(lineProg, lineVAO, lineVBO, center, size, mvp, color, lineWidth);
    };
    auto lines = [&](
        const vector<Eigen::Vector3f>& points,
        array<float, 4> color, float lineWidth
    ) {
        drawLines(lineProg, lineVAO, lineVBO, points, mvp, color, lineWidth);
    };

    if (auto* it = state.asMode<IterationDiffState>(); it && it->iterA >= 0 && it->iterB >= 0) {
        if (it->iterA < (int)state.datasets.size()
         && it->parentCellIdx >= 0
         && it->parentCellIdx < (int)state.datasets[it->iterA].nodes.size())
        {
            auto& parent = state.datasets[it->iterA].nodes[it->parentCellIdx];
            box(parent.pos, parent.size, { COL_ORANGE.x, COL_ORANGE.y, COL_ORANGE.z, 1 }, 1);
        }
        if (it->iterB < (int)state.datasets.size()) {
            auto& newerDS = state.datasets[it->iterB];
            for (int ci = 0; ci < (int)it->childrenIndices.size(); ++ci) {
                int childIdx = it->childrenIndices[ci];
                if (childIdx >= 0 && childIdx < (int)newerDS.nodes.size()) {
                    float lw = (ci == it->activeChild) ? 3.0f : 1.5f;
                    box(newerDS.nodes[childIdx].pos, newerDS.nodes[childIdx].size,
                        { COL_PURPLE.x, COL_PURPLE.y, COL_PURPLE.z, 1 }, lw);
                }
            }
        }
    } else if (state.isMode<PartialCompState>()) {
        int colorIndex = 0;
        for (int si = 0; si < state.numSlots && si < MAX_COMP_SLOTS; ++si) {
            auto& slot = state.slots[si];
            if (!slot.valid || slot.datasetIndex < 0 || slot.nodeIndex < 0) continue;

            auto& compDS = state.datasets[slot.datasetIndex];
            if (slot.nodeIndex >= (int)compDS.nodes.size()) continue;

            auto& cell = compDS.nodes[slot.nodeIndex];
            float lw   = (slot.datasetIndex == state.activeDataset) ? 3.0f : 2.0f;
            box(cell.pos, cell.size, COMP_SLOT_COLORS[colorIndex], lw);
            colorIndex++;
        }
    } else {
        int selA = state.getSelectedCellA();
        int selB = state.getSelectedCellB();
        if (selA >= 0 && selA < (int)dataset.nodes.size()) {
            box(dataset.nodes[selA].pos, dataset.nodes[selA].size,
                { COL_ORANGE.x, COL_ORANGE.y, COL_ORANGE.z, 1 }, 3);
        }
        if (selB >= 0 && selB < (int)dataset.nodes.size()) {
            box(dataset.nodes[selB].pos, dataset.nodes[selB].size,
                { COL_PURPLE.x, COL_PURPLE.y, COL_PURPLE.z, 1 }, 3);
        }
    }

    if (state.flags.showAllWireframes) {
        auto& idx  = activeIndex();
        int   selA = state.getSelectedCellA();
        int   selB = state.getSelectedCellB();
        for (int i : idx) {
            auto& node = dataset.nodes[i];
            if (i == selA || i == selB) continue;
            box(node.pos, node.size, { 0.4f, 0.4f, 0.4f, 0.25f }, 3);
        }
    }

    if (state.flags.isFilterActive && state.flags.showFilterBox) {
        vector<Eigen::Vector3f> filterEdges;
        boxEdgesMinMax(state.flags.filterMin, state.flags.filterMax, filterEdges);
        lines(filterEdges, { COL_GREEN.x, COL_GREEN.y, COL_GREEN.z, 0.6f }, 2);
    }

    if (state.hover.active) {
        float rayLen = (sceneMax - sceneMin).norm() * 0.3f;
        lines({ state.hover.origin, state.hover.origin + state.hover.direction * rayLen },
              { COL_CYAN.x, COL_CYAN.y, COL_CYAN.z, 1 }, 3);
    }

    Eigen::Vector3f origin   = sceneMin;
    float           axisLen  = (sceneMax - sceneMin).norm() * 0.12f;

    lines({ origin, origin + Eigen::Vector3f(axisLen, 0, 0) }, { 0.8f, 0.2f, 0.2f, 0.6f }, 2);
    lines({ origin, origin + Eigen::Vector3f(0, axisLen, 0) }, { 0.2f, 0.8f, 0.2f, 0.6f }, 2);
    lines({ origin, origin + Eigen::Vector3f(0, 0, axisLen) }, { 0.2f, 0.2f, 0.8f, 0.6f }, 2);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

int SDTreeViewer::pickNode(float screenX, float screenY, float viewWidth, float viewHeight) {
    auto& dataset = state.datasets[state.activeDataset];
    auto& idx     = activeIndex();
    if (idx.empty()) return -1;

    Eigen::Matrix4f mvp = OrbitCamera::perspective(0.8f, viewWidth / max(viewHeight, 1.f), 0.01f, 1000)
                        * cam.viewMatrix();

    float bestDistSq = 1e30f;
    int   bestIndex  = -1;

    for (int i : idx) {
        Eigen::Vector3f pos  = dataset.nodes[i].pos;
        Eigen::Vector4f clip = mvp * Eigen::Vector4f(pos.x(), pos.y(), pos.z(), 1);
        if (clip.w() <= 0) continue;

        float projX = (clip.x() / clip.w() * 0.5f + 0.5f) * viewWidth;
        float projY = (clip.y() / clip.w() * 0.5f + 0.5f) * viewHeight;
        float ds    = (projX - screenX) * (projX - screenX) + (projY - screenY) * (projY - screenY);

        if (ds < bestDistSq) { bestDistSq = ds; bestIndex = i; }
    }

    return bestDistSq > 2500 ? -1 : bestIndex;
}

void SDTreeViewer::activateKL() {
    if (auto* n = state.asMode<NormalState>()) {
        if (n->selectedCellA >= 0 && n->selectedCellB >= 0) {
            n->isKlMode = true;
            updateHeatmaps();
        }
    }
}

bool SDTreeViewer::heatmapUV(ImVec2 imgPos, ImVec2 imgSize, float& u, float& v) {
    ImVec2 mouse = ImGui::GetIO().MousePos;
    float  localX = mouse.x - imgPos.x;
    float  localY = mouse.y - imgPos.y;
    if (localX < 0 || localY < 0 || localX >= imgSize.x || localY >= imgSize.y) return false;
    u = localX / imgSize.x;
    v = 1.0f - (localY / imgSize.y);
    return true;
}

void SDTreeViewer::rebuildSlot(int slotIndex) {
    if (slotIndex < 0 || slotIndex >= state.numSlots) return;

    auto& slot = state.slots[slotIndex];
    if (slot.datasetIndex < 0 || slot.nodeIndex < 0) return;

    auto& dtree = state.datasets[slot.datasetIndex].nodes[slot.nodeIndex].dTree;
    slot.leaves.clear();
    dtree->collectLeaves(slot.leaves, state.slotTiling[slotIndex]);
    buildSlotTexture(slotIndex, dtree);
}

vector<int> SDTreeViewer::getSequenceGroups() const {
    vector<int> groups;
    int maxGroup = -1;
    for (auto& d : state.datasets) maxGroup = max(maxGroup, d.groupIndex);
    for (int g = 0; g <= maxGroup; ++g) {
        int count = 0;
        for (auto& d : state.datasets) if (d.groupIndex == g) count++;
        if (count >= 2) groups.push_back(g);
    }
    return groups;
}

vector<int> SDTreeViewer::getGroupMembers(int groupIdx) const {
    vector<int> members;
    for (int i = 0; i < (int)state.datasets.size(); ++i)
        if (state.datasets[i].groupIndex == groupIdx) members.push_back(i);
    return members;
}

void SDTreeViewer::renderUI(GLFWwindow* win, int winW, int winH) {
    (void)win;
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    float topH  = renderSequenceBar(*this, winW);
    float bodyH = winH - topH;

    float settingsW = winW * 0.15f;
    float spatialW  = winW * 0.50f;
    ImGui::SetNextWindowPos({ 0, topH });
    ImGui::SetNextWindowSize({ (float)winW, bodyH });
    ImGui::Begin("##Body", 0, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoBackground);

    ImGui::BeginChild("Settings", { settingsW, bodyH - 10 }, true);
    renderSettingsPanel(*this);
    ImGui::EndChild(); ImGui::SameLine();

    ImGui::BeginChild("Spatial", { spatialW, bodyH - 10 }, true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    renderSpatialPanel(*this);
    ImGui::EndChild(); ImGui::SameLine();

    ImGui::BeginChild("Dir", { 0, bodyH - 10 }, true);
    renderDirectionalPanel(*this);
    ImGui::EndChild();

    ImGui::End();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}


void SDTreeViewer::processKeys(GLFWwindow* win) {
    state.isShiftHeld = (glfwGetKey(win, GLFW_KEY_LEFT_SHIFT)  == GLFW_PRESS) || (glfwGetKey(win, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS);

    auto pressed = [&](int key) -> bool {
        bool current = glfwGetKey(win, key) == GLFW_PRESS;
        bool edge    = current && !state.prevKeys[key];
        state.prevKeys[key] = current;
        return edge;
    };

    if (pressed(GLFW_KEY_RIGHT) || pressed(GLFW_KEY_UP))   navNode(1);
    if (pressed(GLFW_KEY_LEFT)  || pressed(GLFW_KEY_DOWN)) navNode(-1);
    if (pressed(GLFW_KEY_PAGE_UP))                         navNode(100);
    if (pressed(GLFW_KEY_PAGE_DOWN))                       navNode(-100);

    if (pressed(GLFW_KEY_ENTER) || pressed(GLFW_KEY_KP_ENTER)) activateKL();

    if (pressed(GLFW_KEY_ESCAPE)) clearSelection();
    if (pressed(GLFW_KEY_T))      cycleBtcTiling();
    if (pressed(GLFW_KEY_S))      cycleScene();
}

void SDTreeViewer::exportSlotHeatmap(int slotIndex) {
    auto& slot = state.slots[slotIndex];
    if (slot.datasetIndex < 0 || slot.nodeIndex < 0) return;

    auto& dtree = state.datasets[slot.datasetIndex].nodes[slot.nodeIndex].dTree;

    static float grid[HMAP_RES][HMAP_RES];
    dtree->fillGrid(grid, state.slotTiling[slotIndex]);

    char filename[256];
    std::snprintf(filename, 256, "heatmap_%s_node%d.png",
        state.datasets[slot.datasetIndex].name.c_str(), slot.nodeIndex);

    if (PNGExport::exportGrayscalePNG<HMAP_RES>(filename, grid))
        std::cout << "Exported: " << filename << std::endl;
    else
        std::cerr << "Export failed: " << filename << std::endl;
}

// ---- mode / dataset transitions ----

void SDTreeViewer::enterNormalMode() {
    state.mode = NormalState{};
    rebuildPointCloud();
    updateHeatmaps();
}

void SDTreeViewer::enterPartialMode() {
    state.mode = PartialCompState{};
    for (int i = 0; i < (int)state.dsChecked.size(); ++i) {
        if (state.dsChecked[i]) { state.activeDataset = i; break; }
    }
    int curA = state.getSelectedCellA();
    if (curA >= 0 && curA < (int)state.datasets[state.activeDataset].nodes.size()) {
        state.partialXYZ = state.datasets[state.activeDataset].nodes[curA].pos;
    }
    rebuildPointCloud();
    updateHeatmaps();
}

void SDTreeViewer::enterIterationMode(int groupIdx) {
    IterationDiffState iter;
    iter.groupIndex = groupIdx;
    state.mode = std::move(iter);
    auto members = getGroupMembers(groupIdx);
    if (!members.empty()) {
        state.activeDataset = members[0];
        if (state.flags.isFilterActive) recomputeFilter();
    }
    rebuildPointCloud();
    updateHeatmaps();
}

void SDTreeViewer::enterTotalDiff(int before, int after) {
    TotalCompState tc;
    tc.diffBefore    = before;
    tc.diffAfter     = after;
    tc.divResult     = compareStructure(state.datasets[before], state.datasets[after]);
    tc.selectedCellA = state.getSelectedCellA();
    state.mode          = std::move(tc);
    state.activeDataset = after;
    if (state.flags.isFilterActive) recomputeFilter();
    rebuildPointCloud();
    updateHeatmaps();
}

void SDTreeViewer::exitTotalDiff() {
    state.mode = NormalState{};
    rebuildPointCloud();
    updateHeatmaps();
}

// ---- sequence iteration ----

void SDTreeViewer::setSequenceGroup(int groupIdx) {
    auto* iter = state.asMode<IterationDiffState>();
    if (!iter) return;

    *iter = IterationDiffState{};
    iter->groupIndex = groupIdx;
    auto members = getGroupMembers(groupIdx);
    if (!members.empty()) {
        state.activeDataset = members[0];
        rebuildPointCloud();
    }
    updateHeatmaps();
}

void SDTreeViewer::clickIterationButton(int dsIdx, bool shift) {
    auto* iter = state.asMode<IterationDiffState>();
    if (!iter) return;

    if (shift) iter->iterB = dsIdx;
    else       iter->iterA = dsIdx;
    if (iter->iterA >= 0 && iter->iterB >= 0 && iter->iterA > iter->iterB) {
        std::swap(iter->iterA, iter->iterB);
    }
    if (iter->iterA >= 0) {
        state.activeDataset = iter->iterA;
        if (state.flags.isFilterActive) recomputeFilter();
        rebuildPointCloud();
    }
    updateSequenceChildren();
    updateHeatmaps();
}

void SDTreeViewer::toggleShowAllChildren(bool value) {
    if (auto* iter = state.asMode<IterationDiffState>()) {
        iter->showAll = value;
        updateHeatmaps();
    }
}

void SDTreeViewer::setActiveChild(int childIdx) {
    if (auto* iter = state.asMode<IterationDiffState>()) {
        iter->activeChild = childIdx;
        updateHeatmaps();
    }
}

// ---- selection ----

void SDTreeViewer::pickSpatialNode(int idx, bool shift) {
    if (auto* iter = state.asMode<IterationDiffState>()) {
        iter->parentCellIdx = idx;
        cacheSelectedXYZ();
        updateSequenceChildren();
        updateHeatmaps();
    } else if (auto* n = state.asMode<NormalState>()) {
        if (shift) {
            n->selectedCellB = idx;
        } else {
            n->selectedCellA = idx;
            n->selectedCellB = -1;
            n->isKlMode      = false;
        }
        cacheSelectedXYZ();
        updateHeatmaps();
    } else if (auto* p = state.asMode<PartialCompState>()) {
        p->selectedCellA = idx;
        state.partialXYZ = state.datasets[state.activeDataset].nodes[idx].pos;
        cacheSelectedXYZ();
        updateHeatmaps();
    } else if (auto* tc = state.asMode<TotalCompState>()) {
        tc->selectedCellA = idx;
        cacheSelectedXYZ();
        updateHeatmaps();
    }
}

void SDTreeViewer::goToCellA() {
    if (auto* iter = state.asMode<IterationDiffState>()) {
        int targetDS = (iter->iterA >= 0) ? iter->iterA : state.activeDataset;
        iter->parentCellIdx = findNode(state.datasets[targetDS], state.cellAXYZ);
        cacheSelectedXYZ();
        updateSequenceChildren();
        updateHeatmaps();
    } else {
        int newA = findNode(state.datasets[state.activeDataset], state.cellAXYZ);
        state.setSelectedCellAIndex(newA);
        if (auto* n = state.asMode<NormalState>()) {
            n->selectedCellB = -1;
            n->isKlMode      = false;
        }
        cacheSelectedXYZ();
        updateHeatmaps();
    }
}

void SDTreeViewer::goToCellB() {
    if (auto* n = state.asMode<NormalState>()) {
        n->selectedCellB = findNode(state.datasets[state.activeDataset], state.cellBXYZ);
    }
    cacheSelectedXYZ();
    updateHeatmaps();
}

void SDTreeViewer::clearSelection() {
    if (auto* n = state.asMode<NormalState>()) {
        n->selectedCellB = -1;
        n->isKlMode      = false;
    }
    cacheSelectedXYZ();
    updateHeatmaps();
}

// ---- filter ----

void SDTreeViewer::setFilterToScene() {
    auto& dataset = state.datasets[state.activeDataset];
    state.flags.filterMin = dataset.aabbMin;
    state.flags.filterMax = dataset.aabbMax;
}

// ---- slot operations ----

void SDTreeViewer::setSlotTiling(int slotIdx, int tilingIdx) {
    state.slotTiling[slotIdx] = tilingIdx;
    if (tilingIdx < 0) state.insp.valid = false;
    rebuildSlot(slotIdx);
}

void SDTreeViewer::cycleBtcTiling() {
    if (state.numSlots != 1) return;

    // BTC: cycle Avg -> tiling 0 -> tiling 1 -> ... -> Avg.
    // CostQuadTree: toggle Radiance (0) <-> Costs (1).
    auto& slot = state.slots[0];
    if (slot.dirType == BTC && slot.numTilings > 0) {
        int& ti = state.slotTiling[0];
        ti++;
        if (ti >= slot.numTilings) ti = -1;
        if (ti < 0) state.insp.valid = false;
        rebuildSlot(0);
    } else if (slot.dirType == CostQuadTree) {
        int& ti = state.slotTiling[0];
        ti = (ti == 1) ? 0 : 1;
        rebuildSlot(0);
    }
}

void SDTreeViewer::beginKlPicking() {
    state.kl.isActive = true;
    state.kl.slotA    = -1;
    state.kl.slotB    = -1;
}

void SDTreeViewer::cancelKlPicking() {
    state.kl = {};
}

void SDTreeViewer::pickKlSlot(int slotIdx) {
    if (state.kl.slotA < 0) {
        state.kl.slotA = slotIdx;
        return;
    }
    if (state.kl.slotB >= 0 || slotIdx == state.kl.slotA) return;

    state.kl.slotB = slotIdx;
    if (state.numSlots < MAX_COMP_SLOTS) {
        buildKL(state.numSlots,
                state.slots[state.kl.slotA].leaves,
                state.slots[state.kl.slotB].leaves);
        char buf[256];
        std::snprintf(buf, 256, "KL: %s vs %s",
            state.datasets[state.slots[state.kl.slotA].datasetIndex].name.c_str(),
            state.datasets[state.slots[state.kl.slotB].datasetIndex].name.c_str());
        state.slots[state.numSlots].title   = buf;
        state.slots[state.numSlots].nodePos = state.slots[state.kl.slotA].nodePos;
        state.numSlots++;
    }
    state.kl = {};
}

void SDTreeViewer::inspectBin(int slotIdx, float u, float v) {
    if (slotIdx < 0 || slotIdx >= state.numSlots) return;
    auto& slot = state.slots[slotIdx];
    if (slot.dirType == BTC && state.slotTiling[slotIdx] < 0) return;

    auto* leaf = findLeafUV(slot.leaves, u, v);
    if (!leaf) return;

    float area = (leaf->bounds.maxA - leaf->bounds.minA)
               * (leaf->bounds.maxB - leaf->bounds.minB);
    float totalNodeRad = 0;
    for (auto& l : slot.leaves) totalNodeRad += max(l.radiance, 0.f);
    float pct = (totalNodeRad > 0) ? leaf->radiance / totalNodeRad * 100 : 0;
    state.insp = { leaf->radiance,
                   { leaf->bounds.minA, leaf->bounds.minB,
                     leaf->bounds.maxA, leaf->bounds.maxB },
                   area, pct, slotIdx, true };
}

// ---- partial compare ----

void SDTreeViewer::setCheckedDataset(int idx, bool value) {
    if (idx >= (int)state.dsChecked.size()) state.dsChecked.resize(idx + 1, false);
    state.dsChecked[idx] = value;
}

void SDTreeViewer::cycleScene() {
    if (!state.isMode<PartialCompState>()) return;

    std::vector<int> checked;
    for (int i = 0; i < (int)state.dsChecked.size(); ++i)
        if (state.dsChecked[i]) checked.push_back(i);
    if (checked.empty()) return;

    auto pos_it = find(checked.begin(), checked.end(), state.activeDataset);
    int  pos    = (pos_it != checked.end()) ? (int)(pos_it - checked.begin()) : -1;
    pos = (pos + 1) % (int)checked.size();
    state.activeDataset = checked[pos];
    if (state.flags.isFilterActive) recomputeFilter();
    rebuildPointCloud();
}
