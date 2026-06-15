#pragma once

#include <raylib-cpp.hpp>  // Vector3 — plain POD, fine to use in components.
#include <string>

// ECS components. Keep them as plain data (no logic, no behaviour). Systems
// operate on them. Heavy GPU resources (Model, Texture) are NOT stored here by
// value -- store a handle/id and look them up in an asset manager.
namespace Long {

// World transform of an entity.
struct Transform {
    raylib::Vector3 position = {0, 0, 0};
    raylib::Vector3 rotationEuler = {0, 0, 0}; // degrees, yaw/pitch/roll
    raylib::Vector3 scale = {1, 1, 1};
};

// Which grid cell this entity occupies (for tile-based placement).
//struct GridPosition {
//    int x = 0;
//    int z = 0;
//};

// Visual: references a model in the AssetManager by id (not the model itself).
struct MeshRenderer {
    int modelId = -1;
    raylib::Color tint = raylib::Color::White();
    bool visible = true;
};

// Human-readable name (handy in the editor's entity list / inspector).
struct Name {
    std::string value;
};

//// Tag components (empty) -- used to mark entities for systems/queries.
//struct StaticTile {};   // part of the level geometry
//struct Selected {};     // currently selected in the editor

} // namespace Long
