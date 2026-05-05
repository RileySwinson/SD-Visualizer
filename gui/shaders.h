#pragma once

#include <GL/glew.h>

// Vertex/fragment shaders for the spatial point cloud (radius-clipped circles).
extern const char* POINT_VS;
extern const char* POINT_FS;

// Vertex/fragment shaders for line geometry (uniform color).
extern const char* LINE_VS;
extern const char* LINE_FS;

GLuint compileShader(GLenum type, const char* source);
GLuint linkProgram(const char* vertSrc, const char* fragSrc);
