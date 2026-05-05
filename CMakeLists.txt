cmake_minimum_required(VERSION 3.16)
project(SDTreeViewer LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Dependencies
find_package(OpenGL REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(Eigen3 3.3 REQUIRED)
find_package(ZLIB REQUIRED)

include(FetchContent)

# Dear ImGui
FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.91.8
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(imgui)

add_library(imgui_lib STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui_lib PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui_lib PUBLIC glfw GLEW::GLEW OpenGL::GL)

# Source layout — see /readme or /design notes for rationale.
set(VISUALIZER_SOURCES
    main.cpp

    # Pure compute (no GL, no ImGui)
    domain/compute/filter.cpp
    domain/compute/nodeQueries.cpp
    domain/compute/pointCloudBuild.cpp
    domain/compute/rasterize.cpp
    domain/compute/structureCompare.cpp

    # Loaders: probe + dispatch + per-format readers
    domain/loaders/dispatch.cpp
    domain/loaders/probe.cpp
    domain/loaders/spatialLoader.cpp

    # State: AppState container + variant Mode
    domain/state/appState.cpp

    # Per-structure directional readers + interface
    domain/directional/btcTree.cpp
    domain/directional/costQuadtreeTree.cpp
    domain/directional/directionalLoader.cpp
    domain/directional/quadtreeTree.cpp

    # Per-structure spatial readers
    domain/spatial/sdtreeSpatial.cpp

    # GUI primitives (GL ownership; no application state)
    gui/geometryRenderer.cpp
    gui/glResources.cpp
    gui/orbitCamera.cpp
    gui/shaders.cpp

    # Per-panel ImGui code
    gui/panels/sequenceBar.cpp
    gui/panels/settingsPanel.cpp
    gui/panels/spatialPanel.cpp
    gui/panels/directionalPanel.cpp

    # The viewer (state + ImGui frame).
    viewer/sdtreeViewer.cpp
)

add_executable(visualizer ${VISUALIZER_SOURCES})
target_include_directories(visualizer PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(visualizer PRIVATE imgui_lib Eigen3::Eigen ZLIB::ZLIB)
