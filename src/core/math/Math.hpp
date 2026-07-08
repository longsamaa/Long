#pragma once
#ifndef _LONG_MATH_HPP_
#define _LONG_MATH_HPP_

// Neutral math entry point. Everything in the engine includes ONLY this header
// and uses Long::Math::{Vec3, Mat4, ...}; the actual implementation is selected
// at compile time by a CMake define (target_compile_definitions ... PUBLIC):
//
//   LONG_MATH_RAYLIB  -> raylib/raymath backend  (Vec3 = ::Vector3, ...)
//   LONG_MATH_GLM     -> glm backend             (Vec3 = glm::vec3, ...)
//
// Backends are typedef + inline forwarders, so with optimization the whole
// Math:: layer compiles away -- calling Math::Add is identical to calling the
// backend function directly (zero-cost abstraction). See MathRaylib.hpp.

#if defined(LONG_MATH_RAYLIB)
    #include "core/math/backend/MathRaylib.hpp"
#elif defined(LONG_MATH_GLM)
    #include "core/math/backend/MathGLM.hpp"
#else
    #error "No math backend selected. Define LONG_MATH_RAYLIB or LONG_MATH_GLM (CMake: -DLONG_MATH=raylib|glm)."
#endif

#endif // !_LONG_MATH_HPP_
