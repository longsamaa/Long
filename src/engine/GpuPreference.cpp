// Prefer the discrete GPU (NVIDIA / AMD) over the integrated Intel iGPU.
//
// On laptops/desktops with switchable graphics, the driver looks for these
// exported symbols in the running executable and, if present and non-zero,
// runs the app on the high-performance discrete GPU. These MUST be exported
// from the final .exe (not a DLL), with C linkage and the exact names below.
//
//   - NvOptimusEnablement                     -> NVIDIA Optimus
//   - AmdPowerXpressRequestHighPerformance    -> AMD PowerXpress / Enduro
//
// References:
//   https://docs.nvidia.com/gameworks/content/technologies/desktop/optimus.htm
//   AMD PowerXpress application notes
#if defined(_WIN32)

extern "C" {
    __declspec(dllexport) unsigned long NvOptimusEnablement = 1;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

#endif
