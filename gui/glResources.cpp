#include "glResources.h"

// AI Generated, thanks claude code

void GlVertexArray::create()  { glGenVertexArrays(1, &mHandle); }
void GlVertexArray::destroy() { if (mHandle) glDeleteVertexArrays(1, &mHandle); mHandle = 0; }
GlVertexArray::~GlVertexArray() { destroy(); }

void GlBuffer::create()  { glGenBuffers(1, &mHandle); }
void GlBuffer::destroy() { if (mHandle) glDeleteBuffers(1, &mHandle); mHandle = 0; }
GlBuffer::~GlBuffer() { destroy(); }

void GlTexture::create()  { glGenTextures(1, &mHandle); }
void GlTexture::destroy() { if (mHandle) glDeleteTextures(1, &mHandle); mHandle = 0; }
GlTexture::~GlTexture() { destroy(); }

void GlRenderbuffer::create()  { glGenRenderbuffers(1, &mHandle); }
void GlRenderbuffer::destroy() { if (mHandle) glDeleteRenderbuffers(1, &mHandle); mHandle = 0; }
GlRenderbuffer::~GlRenderbuffer() { destroy(); }

void GlFramebuffer::create()  { glGenFramebuffers(1, &mHandle); }
void GlFramebuffer::destroy() { if (mHandle) glDeleteFramebuffers(1, &mHandle); mHandle = 0; }
GlFramebuffer::~GlFramebuffer() { destroy(); }

// Programs are created via linkProgram, not gen* — `create()` is a no-op.
void GlProgram::create()  {}
void GlProgram::destroy() { if (mHandle) glDeleteProgram(mHandle); mHandle = 0; }
GlProgram::~GlProgram() { destroy(); }
