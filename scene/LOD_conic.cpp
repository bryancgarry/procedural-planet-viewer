//
//  LOD_conic.cpp
//  JHU_605.667
//
//  Created by Bryan Garry.
//

#include "scene/LOD_conic.hpp"

#include <cmath>
#include <iostream>

namespace cg
{

LODConic::LODConic(float bottom_radius,
                   float top_radius,
                   const Point3& world_pos,
                   const std::string& name,
                   int32_t position_loc,
                   int32_t normal_loc)
    : world_pos_(world_pos), name_(name)
{
    node_type_ = SceneNodeType::GEOMETRY;

    // LOD 0 — High detail: 8 sides, 4 stacks
    lod_[0] = std::make_shared<ConicSurface>(
        bottom_radius, top_radius, 8, 4, position_loc, normal_loc);

    // LOD 1 — Medium detail: 5 sides, 2 stacks
    lod_[1] = std::make_shared<ConicSurface>(
        bottom_radius, top_radius, 5, 2, position_loc, normal_loc);

    // LOD 2 — Low detail: 3 sides, 1 stack
    lod_[2] = std::make_shared<ConicSurface>(
        bottom_radius, top_radius, 3, 1, position_loc, normal_loc);}

void LODConic::draw(SceneState& scene_state)
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

    if (lod_level != last_lod_)
    {
        const char* labels[] = { "HIGH (8x4)", "MEDIUM (5x2)", "LOW (3x1)" };
        std::cout << "[LOD] \"" << name_ << "\""
                  << " dist=" << dist
                  << " -> " << labels[lod_level] << "\n";
        last_lod_ = lod_level;
    }

    lod_[lod_level]->draw(scene_state);
}

void LODConic::update(SceneState& scene_state)
{
    // Nothing to update
}

} // namespace cg
