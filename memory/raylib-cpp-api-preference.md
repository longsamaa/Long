---
name: raylib-cpp-api-preference
description: Always prefer raylib-cpp methods over C functions; which calls have no method wrapper
metadata:
  type: feedback
---

User repeatedly insists on using raylib-cpp API instead of the C API ("xài api của raylib cpp", "đổi lại thành raylib cpp"). Convert `Vector3Add/Subtract/Scale/Normalize/CrossProduct/DotProduct/Distance` → `.Add/.Subtract/.Scale/.Normalize/.CrossProduct/.DotProduct/.Distance`, `GetRayCollisionBox(ray,box)` → `raylib::Ray(ray).GetCollision(box)`, drawing → `.DrawLine3D/.DrawSphere`, `box.Draw()`.

**Why:** readability + consistency; he's learning the engine and wants one idiom.

**How to apply:** when a Camera/Ray field is a plain `::Vector3`, wrap it: `raylib::Vector3(field).Add(...)`. These have NO method wrapper in this raylib-cpp version — keep them as C `::` calls: `Vector3Min/Vector3Max/Vector3Transform`, `DrawCylinderEx`, `GetWorldToScreen`, `DrawLineEx`. See [[material-ownership-no-double-free]].
