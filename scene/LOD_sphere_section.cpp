//
//  LOD_sphere_section.cpp
//  JHU_605.667
//
//  Created by Bryan Garry.
//

#include "scene/LOD_sphere_section.hpp"
#include <cmath>
#include <iostream>

namespace cg
{

LODSphereSection::LODSphereSection(float radius,
                                   const Point3 &world_pos,
                                   const std::string &name,
                                   int32_t position_loc,
                                   int32_t normal_loc)
    : world_pos_(world_pos), name_(name), radius_(radius)
{
    node_type_ = SceneNodeType::GEOMETRY;

    // Build the three LOD meshes up front at construction time.

    // LOD 0 — High detail: 16 lat x 32 lon divisions
    lod_[0] = std::make_shared<SphereSection>(
        -90.0f, 90.0f, 16,
        -180.0f, 180.0f, 32,
        radius, position_loc, normal_loc);

    // LOD 1 — Medium detail: 8 lat x 16 lon divisions
    lod_[1] = std::make_shared<SphereSection>(
        -90.0f, 90.0f, 8,
        -180.0f, 180.0f, 16,
        radius, position_loc, normal_loc);

    // LOD 2 — Low detail: 4 lat x 8 lon divisions
    lod_[2] = std::make_shared<SphereSection>(
        -90.0f, 90.0f, 4,
        -180.0f, 180.0f, 8,
        radius, position_loc, normal_loc);
}

void LODSphereSection::draw(SceneState &scene_state)
{
    // Compute distance from camera to sphere world center
    float dx = scene_state.camera_position.x - world_pos_.x;
    float dy = scene_state.camera_position.y - world_pos_.y;
    float dz = scene_state.camera_position.z - world_pos_.z;
    float dist = std::sqrt(dx * dx + dy * dy + dz * dz);

    // Select LOD level based on distance
    int lod_level;
    if (dist < DIST_HIGH)
    {
        lod_level = 0;
    }
    else if (dist < DIST_MEDIUM)
    {
        lod_level = 1;
    }
    else
    {
        lod_level = 2;
    }

    // Print only when LOD level changes
    if(lod_level != last_lod_)
    {
        const char* labels[] = { "HIGH (16x32)", "MEDIUM (8x16)", "LOW (4x8)" };
        std::cout << "[LOD] \"" << name_ << "\""
                  << " dist=" << dist
                  << " -> " << labels[lod_level] << "\n";
        last_lod_ = lod_level;
    }

    // Draw the selected LOD mesh
    lod_[lod_level]->draw(scene_state);
}

void LODSphereSection::update(SceneState &scene_state)
{
    // Nothing to update
}

} // namespace cg
