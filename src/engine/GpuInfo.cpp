#include "engine/GpuInfo.h"

#include "rlgl.h"

// OpenGL enum values (avoid pulling in a full GL header).
#ifndef GL_VENDOR
#define GL_VENDOR   0x1F00
#define GL_RENDERER 0x1F01
#define GL_VERSION  0x1F02
#endif

// glGetString signature. We fetch the function pointer through raylib's
// rlGetProcAddress so we don't have to link a GL loader ourselves.
typedef const unsigned char* (*PFNGLGETSTRINGPROC)(unsigned int name);

static std::string QueryString(PFNGLGETSTRINGPROC glGetString, unsigned int name) {
    if (!glGetString) {
        return "(unavailable)";
    }
    const unsigned char* s = glGetString(name);
    return s ? reinterpret_cast<const char*>(s) : "(null)";
}

GpuInfo GpuInfo::Query() {
    auto glGetString = reinterpret_cast<PFNGLGETSTRINGPROC>(
        rlGetProcAddress("glGetString"));

    GpuInfo info;
    info.vendor   = QueryString(glGetString, GL_VENDOR);
    info.renderer = QueryString(glGetString, GL_RENDERER);
    info.version  = QueryString(glGetString, GL_VERSION);
    return info;
}
