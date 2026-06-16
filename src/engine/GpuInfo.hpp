#pragma once

#include <string>

// Info about the OpenGL renderer the app is currently running on. Read it
// AFTER the window / GL context is created (i.e. after Application is built).
struct GpuInfo {
    std::string vendor;    // e.g. "NVIDIA Corporation"
    std::string renderer;  // e.g. "NVIDIA GeForce RTX 3070 Ti/PCIe/SSE2"
    std::string version;   // e.g. "3.3.0 NVIDIA 552.xx"

    // Queries the current GL context. Cache the result; don't call per-frame.
    static GpuInfo Query();
};
