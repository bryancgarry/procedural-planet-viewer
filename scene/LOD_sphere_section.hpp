//
//  LOD_sphere_section.hpp
//  JHU_605.667
//
//  Created by Bryan Garry.
//
//  LOD selection (distance from camera to sphere center):
//      HIGH   (< 50 units)  : num_lat=16, num_lon=32
//      MEDIUM (50-120 units): num_lat=8,  num_lon=16
//      LOW    (> 120 units) : num_lat=4,  num_lon=8

#ifndef __SCENE_LOD_SPHERE_NODE_HPP__
#define __SCENE_LOD_SPHERE_NODE_HPP__

#include "scene/scene_node.hpp"
#include "scene/sphere_section.hpp"
#include "geometry/point3.hpp"

#include <memory>
#include <string>

namespace cg
{

class LODSphereSection : public SceneNode
{
  public:
    /**
     * Constructs a sphere with 3 pre-built LOD levels.
     * @param  radius        Radius of the sphere
     * @param  world_pos     World-space center of the sphere (used for distance calc)
     * @param  name          Name used in LOD printouts
     * @param  position_loc  Shader position attribute location
     * @param  normal_loc    Shader normal attribute location
     */
    LODSphereSection(float          radius,
                  const Point3  &world_pos,
                  const std::string &name,
                  int32_t        position_loc,
                  int32_t        normal_loc);

    /**
     * Draw the appropriate LOD level based on distance from the camera.
     * Prints a message when the active LOD level changes.
     */
    void draw(SceneState &scene_state) override;

    // Update
    void update(SceneState &scene_state) override;

  private:
    // Three LOD levels: 0 = high, 1 = medium, 2 = low
    std::shared_ptr<SphereSection> lod_[3];

    // World-space position used for distance calculation
    Point3 world_pos_;

    // Name for culling printout
    std::string name_;

    // Radius stored for reference
    float radius_;

    // Tracks the last active LOD to print only on change
    int last_lod_ = -1;

    // Distance thresholds for LOD selection
    static constexpr float DIST_HIGH = 50.0f;
    static constexpr float DIST_MEDIUM = 120.0f;
};

} // namespace cg

#endif
