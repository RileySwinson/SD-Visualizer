Maybe I'll make a better ReadMe later. 

For now, here's a minimal explanation of features. 

Accepts dumps from Rath's path guider (only tested on radiance mode, but see no reason it shouldn't work with any mode, or even custom modes, as long as the SD tree remains the same format)
Accepts dumps from BTC (except I wrote the dump code, so probably can't get that working without using my code. I'll eventually link it here.)

# Building

In same directory, place
main.cpp
pngExport.h
CMakeLists.txt

Make a build dir and configure CMake, then build.

ImGui gets installed during CMake configuration.

Assumes: 
CPP20++ or later, and a compiler
CMakkke 3.16+

That the following packages/libraries are installed:
OpenGL
GLFW 3.3+
FLEW
Eigen3 3.3+

This install should cover all packages:
`sudo apt install cmake build-essential libglfw3-dev libglew-dev libeigen3-dev libgl-dev`

# Inputs

Run in the build folder with the format


./visualizer INPUTS

where INPUTS are a comma-seperated arbitrary number of inputs in one of two forms:

Form 1: Direct Path to Single Dump 
<path-to-dump>

Form 2: List of subsequence dumps

[<path-to-dump-file><sequence-prefix>00, <path-to-dump-file><sequence-prefix>01, <path-to-dump-file><sequence-prefix>02, <path-to-dump-file><sequence-prefix>03]

An arbitrary number of files can be provided in a list; however, they must be labeled with the same prefix, and must be provided in order from least resolved dump -> most resolved dump. 
The dumps do not have to start with the first iteration, nor end with the last iteration, or include every iteration between, so long as each subsequent dump is a refinement of the previous.
The dumps, if provided in sequence, must have filenames indexed in incrementing natural numbers.
The spaces are optional but the commas and encasing [] are mandatory.
Any dumps from the BTC structure must have "btc" somewhere in the filename (case-insensitive).

For example, assuming all the files exist and are valid dumps in sequence, the following is valid:

`./visualizer ../r0cketship-dump-btc.sdt, [../kitchen-vanilla-2047-dump-01.sdt, ../kitchen-vanilla-2047-dump-04.sdt, ../kitchen-vanilla-2047-dump-05.sdt, ../kitchen-vanilla-2047-dump-06.sdt, ../kitchen-vanilla-2047-dump-08.sdt], ../dump-outside-scene.sdt[../kitchen-btc-511-dump-02.sdt, ../kitchen-btc-511-dump-05.sdt, ../kitchen-btc-511-dump-06.sdt]`


# Features

scroll to zoom, click to rotate, right click to pan 
three pannels: Left pannel is settings; middle pannel is spatial visualizer; right pannel is directional visualizer.
at the top are the recognizes uploaded sequences. clicking the items in sequence displays that iteration in the spatial visualizer.

click in the spatial domain to select a cell. its directional heatmap is rendered in the corner. 
the heatmap is on a logarithmic scale, with the min/max being the min/max cache values of that directional component.

The spatial visualization renders each spatial cell as a point in the center of its bounding box. 
The darker the dot, the less radiance contained by that spatial cell.

NOTE: The spatial rendering is independent of the selection logic. 
In other words, you can have a different spatial visualization on the screen than what your selection shows. 
Select the relevant dump from the left in normal mode, or from the top in sequence mode, to switch the visualized spatial dump.

## Setting Options: 

There are three modes with different features and settings. 

NORMAL MODE: 

Cell A and Cell B are selected in the spatial domain with click and left click respectivley.
The (x,y,z) values on the left side can be edited for both cells. The spatial cell containing that coordinate will get selected from the spatial domain. 

When both Cell A and Cell B are selected, the top heatmap is for Cell A and the bottom heatmap is for Cell B. 
The third map is a KL symetric divergence map between them.

All uploaded dumps are listed in the settings pannel. Select different datasets from here to see them in the spatial domain.

PARTIAL DIFF:

Select up to 8 spatial dumps from the left pannel (any more, and it will simply not work).
Upon clicking "compare," the coordinates written in the box for Cell A will be used to find the spatial cell containg those coordinates in ALL selected spatial cells. 
The heatmaps for each of these spatial domains (in order from top-bottom selection) gets displayed on the right. 
Each spatial domain has a rendered bounding box, with a unique color identifier for the heatmap.

The button at the top states "Create KL Map."
Click this, then clik on two heatmaps. It will create a KL divergence map between these. 
NOTE: A maximum of 8 heatmaps can be displayed, including KL maps. Yes you can make KL maps of KL maps if you want. If it ever exeeds 8 heatmaps, the next ones will simply not generate.

TOTAL DIFF: 

This one is kinda a secret mode because it's not listed under modes. To access it, use the sequence selection at the top of the screen.
Click on one of the iterations, then SHIFT-CLICK another iteration in the same sequence. 
The spatial visualization will then ONLY show cells that were split or pruned between the earlier and later iterations.
Note that pruning should never happen in either Quadtree or BTC. 
Statistics are at the top of the screen. 

SEQUENCE:

Sequence mode has a "group" option, which selects which sequence you wish to use. This is only relevant if you have multiple.
Click one iteration from the list, and shift-click another iteration. 
Then, whatever you selct for Cell A, you see all the subdivided chidlren of that node highlighted in purple. 
There will be two heatmaps: One for the parent, and one for the children (subdivided) cells. 
Each child has a number above the heatmap. 
Select those numbers to switch between the spatial cells (i.e., change your seletion for Cell B). 
You can create a KL map here as you can in PARTIAL DIFF mode. 

Ther is a setting on the left: Show heatmaps for all children. 
This does what it says. 
If this would create more than 7 heatmaps, this option is unselectable.

IN ALL MODES: 

There is a "filter" option. This option allows for rendering a subset of the scene. It persists across different spatial domains. 
It is a generous filter, meaning any spatial cells which overlap with the provided filtered domain will get displayed.
the bounding box can go in min-max or max-min (x,y,z) (x,y,z) formats.
The "scene bounds" option sets the filter to the bounds of the scene. 
Hitting apply filter applies it. The filter is automatically applied when switching between scenes.

There is a "show all cells" option. This shows the wirefram of each spatial cells bounding box. Discouraged for full scenes; intended to be used alongside filter.
There is a "show filter box" option. This shows a live visualization of where in the spatial domain the filter is selecting. 

IN THE DIRECTIONAL DOMAIN: 
Select any leaf node in the directional component to see its radaince and UV coordinates and area.
For BTC tilings, there are buttons above the heatmap. These buttons switch between the different offset tilings. 
The "SUM" button displays the sum of all the tilings. You cannot select cells on the SUM page, but can on all the tiling pages.

There is a "Export Heatmap" button. This exports the PDF in Linear Greyscale with white = minimum radiance and black = maximum radiance. 
These min and max radiance values are dumped as a CSV file.
I haven't tested export on BTC, but i expect it will work fine. 
The export goes into the build directory (dont @ me).

The PDF integral is listed below. If it is not within a 0.01 boundry of integrating to 1, it turns red.

Hover over any heatmap to see what direction that UV sample relates to in the spatial domain. 
You can also hover over KL maps but I have no idea which cell it uses to show you the direction because it was basically an unintended side-effect.
