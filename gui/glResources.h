#pragma once

#include <GL/glew.h>

// AI Generated, thanks claude code

// Move-only RAII wrappers for raw OpenGL handles.
//
// Each wrapper owns one handle, deletes it on destruction, and exposes the
// raw `GLuint` via implicit conversion so existing GL calls work unchanged.
//
// GLEW exposes the gl{Gen,Delete}* functions as runtime-loaded function
// pointers (e.g. `__glewDeleteFramebuffers`), so they cannot be used as
// non-type template arguments. Each wrapper therefore has its own
// destructor body rather than parameterising over the deleter.

#define VIZ_GL_HANDLE_DECL(ClassName)                                       \
    class ClassName {                                                       \
    public:                                                                 \
        ClassName() = default;                                              \
        explicit ClassName(GLuint h) : mHandle(h) {}                        \
        ~ClassName();                                                       \
        ClassName(ClassName&& o) noexcept : mHandle(o.mHandle) { o.mHandle = 0; } \
        ClassName& operator=(ClassName&& o) noexcept {                      \
            if (this != &o) { destroy(); mHandle = o.mHandle; o.mHandle = 0; } \
            return *this;                                                   \
        }                                                                   \
        ClassName(const ClassName&)            = delete;                    \
        ClassName& operator=(const ClassName&) = delete;                    \
        GLuint handle() const { return mHandle; }                           \
        operator GLuint() const { return mHandle; }                         \
        void create();                                                      \
        void destroy();                                                     \
    private:                                                                \
        GLuint mHandle = 0;                                                 \
    };

VIZ_GL_HANDLE_DECL(GlVertexArray)
VIZ_GL_HANDLE_DECL(GlBuffer)
VIZ_GL_HANDLE_DECL(GlTexture)
VIZ_GL_HANDLE_DECL(GlRenderbuffer)
VIZ_GL_HANDLE_DECL(GlFramebuffer)
VIZ_GL_HANDLE_DECL(GlProgram)

#undef VIZ_GL_HANDLE_DECL
