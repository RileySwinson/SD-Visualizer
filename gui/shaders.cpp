#include "shaders.h"

// Point shader renders the spatial leaves as colored circles.
// `gl_PointCoord` gives [0,1]^2 within the point sprite. The dot test creates
// a circular mask (radius 0.5 in normalized space).
const char* POINT_VS = R"(#version 330 core
    layout(location=0) in vec3 aPos;
    layout(location=1) in vec4 aCol;

    uniform mat4  uMVP;
    uniform float uPointSize;
    uniform float uRefDist;

    out vec4 vCol;

    void main() {
        gl_Position  = uMVP * vec4(aPos, 1);
        gl_PointSize = max(1.5, uPointSize * uRefDist / gl_Position.w);
        vCol         = aCol;
    }
)";

const char* POINT_FS = R"(#version 330 core
    in  vec4 vCol;
    out vec4 FC;

    void main() {
        vec2 c = gl_PointCoord - vec2(0.5);
        if (dot(c, c) > 0.25) discard;
        FC = vCol;
    }
)";

// Line shader transforms vertices by MVP, applies a uniform color.
// Used for bounding-box wireframes, axis indicators, hover direction ray, etc.
const char* LINE_VS = R"(#version 330 core
    layout(location=0) in vec3 aPos;

    uniform mat4 uMVP;
    uniform vec4 uColor;

    out vec4 vCol;

    void main() {
        gl_Position = uMVP * vec4(aPos, 1);
        vCol        = uColor;
    }
)";

const char* LINE_FS = R"(#version 330 core
    in  vec4 vCol;
    out vec4 FC;

    void main() {
        FC = vCol;
    }
)";

GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, 0);
    glCompileShader(shader);
    return shader;
}

GLuint linkProgram(const char* vertSrc, const char* fragSrc) {
    GLuint vert = compileShader(GL_VERTEX_SHADER,   vertSrc);
    GLuint frag = compileShader(GL_FRAGMENT_SHADER, fragSrc);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert); glAttachShader(prog, frag);
    glLinkProgram(prog); glDeleteShader(vert); glDeleteShader(frag);
    return prog;
}
