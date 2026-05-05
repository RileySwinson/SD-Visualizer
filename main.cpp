/**
 * @file main.cpp
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2026-05-06
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#include "globals/uiColors.h"
#include "domain/loaders/dispatch.h"
#include "viewer/sdtreeViewer.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <cctype>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

// One file to load, with the sequence-group it belongs to (-1 = standalone).

/**
 * @brief Stores one file to loa with the sequence group it belongs to.
 * 
 * Produced by @ref parseDumpArgs() when expanding the bracket argumenats into individual files.
 */
struct ParsedDumpArg {
    std::string filename; ///< Path to .sdt dump
    int         groupIndex; ///< Index of the sequence in the group, 0-indexed
};

/**
 * @brief Helpers for CLI argument parsing. Is an anonymous namespace.
 */
namespace {

// Trim ASCII whitespace from both ends.
/**
 * @brief Trip ASCII whitespace from both ends of @p s
 * 
 * @param s The String to trip 
 * @return std::string The trimmed string
 */
std::string trim(std::string s) {
    while (!s.empty() && std::isspace((unsigned char)s.front())) s.erase(s.begin());
    while (!s.empty() && std::isspace((unsigned char)s.back()))  s.pop_back();
    return s;
}

// Split on commas at bracket-depth zero.
/**
 * @brief Split on commanse
 * 
 * @param s The string to split
 * @return std::vector<std::string> A vetor of parsed filenames
 */
std::vector<std::string> splitTopLevelCommas(const std::string& s) {
    std::vector<std::string> out;
    int depth = 0;
    
    std::string current;
    for (char c : s) {
        
        if      (c == '[') depth++;
        else if (c == ']') depth--;

        if (c == ',' && depth == 0) { 
            out.push_back(current); current.clear(); 
        }

        else { 
            current.push_back(c); 
        }
    }

    out.push_back(current);
    return out;
}

/**
 * @brief Parse the CLI arguments into a flat list of filenames to load.
 * 
 * Three CLI forms are accepted, in any commbination: 
 * 1) standaole: foo.sdt
 * 2) bracket sequence: [foo-00.sdt, foo-01.dst, foo]
 * 3) prefix-postfix syntaxtic suggar foo-[00,01,02].sdt
 * 
 * Spaces do not matter; it's comma-seperated
 * 
 * @param argc Argument count from @ref main()
 * @param argv Argument vector from @ref main() ; tokens are space-sepertaed so care is needed to manage spaced
 * @return std::vector<ParsedDumpArg> A unique @ref ParsedDumpArg per filename. 
 */
std::vector<ParsedDumpArg> parseDumpArgs(int argc, char** argv) {
    std::string joined;

    // join all args into one string
    for (int i = 1; i < argc; ++i) {
        if (!joined.empty()) joined.push_back(' ');
        joined += argv[i];
    }

    std::vector<ParsedDumpArg> out;
    int nextGroup = 0; // the index of the next group

    // loop over each input by splitting at commas not in brackets 
    for (auto& itemRaw : splitTopLevelCommas(joined)) {
        std::string item = trim(itemRaw);
        if (item.empty()) continue;

        auto leftBracket = item.find('[');
        auto rightBracket = item.rfind(']');

        if (leftBracket != std::string::npos && rightBracket != std::string::npos && leftBracket < rightBracket) {
            std::string prefix  = item.substr(0, leftBracket);
            std::string content = item.substr(leftBracket + 1, rightBracket - leftBracket - 1);
            std::string suffix  = item.substr(rightBracket + 1);

            int groupId = nextGroup++; // increment group number after assigning to groupdId
            
            for (auto& postfixRaw : splitTopLevelCommas(content)) {
                
                std::string postfix = trim(postfixRaw);
                if (postfix.empty()) continue;

                out.push_back({ prefix + postfix + suffix, groupId });
            }
    
        } else if (leftBracket != std::string::npos || rightBracket != std::string::npos) {
            std::cerr << "Mismatched brackets in argument: " << item << "\n";
        } else {
            out.push_back({ item, -1 }); // -1 for singular files
        }
    }

    return out;
}

} // namespace

/**
 * @brief Initializes GLFW/GLEW/ImGUI and runs the viewer loop.
 *  
 * @return 0 on normal exit, 1 on failure 
 */
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <inputs>\n"
                  << "  Inputs are comma-separated, mixing any of the following synatex:\n"
                  << "    foo.sdt                                standalone dump\n"
                  << "    [foo-00.sdt, foo-01.sdt, foo-02.sdt]   sequence (full paths)\n"
                  << "    foo-[00, 01, 02].sdt                   sequence (prefix-postfix)\n"
                  << "  The bracket groups must be sequential iterations from the same render.\n";
        return 1;
    }

    registerAllLoaders();

    SDTreeViewer sdTreeViewer;

    // Identifies the relevant SDT structures and loads accordingly
    for (auto const& arg : parseDumpArgs(argc, argv)) {

        auto loaded = dispatchLoad(arg.filename);
       
        if (loaded) {
            loaded->groupIndex = arg.groupIndex;
            sdTreeViewer.state.datasets.push_back(std::move(*loaded));
        } else {
            std::cerr << "Failed to load: " << arg.filename << std::endl;
        }
    }

    if (sdTreeViewer.state.datasets.empty()) {
        std::cerr << "No valid files loaded.\n";
        return 1;
    }

    // GLFW, GLEW initialization 

    if (!glfwInit()) return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(1600, 900, "SD-Tree Explorer", nullptr, nullptr);
    if (!window) { glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "GLEW initialization failed.\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    // ImGUI Initialization

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImFontConfig fontConfig;
    fontConfig.SizePixels = 18;
    ImGui::GetIO().Fonts->AddFontDefault(&fontConfig);

    ImGui::StyleColorsDark();
    auto& style = ImGui::GetStyle();
    style.WindowRounding = 4;
    style.FrameRounding  = 3;
    style.Colors[ImGuiCol_WindowBg]      = { COL_BG.x + .01f, COL_BG.y + .01f, COL_BG.z + .01f, 1 };
    style.Colors[ImGuiCol_ChildBg]       = { COL_BG.x, COL_BG.y, COL_BG.z, 1 };
    style.Colors[ImGuiCol_FrameBg]       = { .17f, .16f, .2f, 1 };
    style.Colors[ImGuiCol_CheckMark]     = COL_ORANGE;
    style.Colors[ImGuiCol_Button]        = { .2f, .18f, .25f, 1 };
    style.Colors[ImGuiCol_ButtonHovered] = { COL_ORANGE.x, COL_ORANGE.y, COL_ORANGE.z, .6f };
    style.Colors[ImGuiCol_Text]          = COL_TEXT;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    //  begin visualizer loop

    sdTreeViewer.init();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        sdTreeViewer.processKeys(window);

        int framebufferW, framebufferH;
        glfwGetFramebufferSize(window, &framebufferW, &framebufferH);
        glViewport(0, 0, framebufferW, framebufferH);
        glClearColor(COL_BG.x, COL_BG.y, COL_BG.z, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        sdTreeViewer.renderUI(window, framebufferW, framebufferH);
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
