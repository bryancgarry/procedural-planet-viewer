//
//  LOD_conic.hpp
//  JHU_605.667
//
//  Created by Bryan Garry.
//
//  LOD selection (distance from camera to world_pos):
//      HIGH   (< 60 units)  : num_sides=8,  num_stacks=4
//      MEDIUM (60-150 units): num_sides=5,  num_stacks=2
//      LOW    (> 150 units) : not drawn (trunk invisible at distance)

#ifndef __SCENE_LOD_CONIC_HPP__
#define __SCENE_LOD_CONIC_HPP__

#include "scene/scene_node.hpp"
#include "scene/conic.hpp"
#include "geometry/point3.hpp"

#include <memory>
#include <string>

namespace cg
{

class LODConic : public SceneNode
{
public:
    /**
     * Constructs a conic surface with 3 pre-built LOD levels.
     * ConicSurface is always unit height (z = -0.5 to 0.5) —
     * use a TransformNode above this to scale to actual trunk height.
     *
     * @param  bottom_radius  Radius at the base (unit scale)
     * @param  top_radius     Radius at the top (unit scale)
     * @param  world_pos      World-space position used for distance calculation
     * @param  name           Name used in LOD printouts
     * @param  position_loc   Shader position attribute location
     * @param  normal_loc     Shader normal attribute location
     */
    LODConic(float              bottom_radius,
             float              top_radius,
             const Point3&      world_pos,
             const std::string& name,
             int32_t            position_loc,
             int32_t            normal_loc);

    /**
     * Draw the appropriate LOD level based on distance from camera.
     * Prints a message when the active LOD level changes.
     */
    void draw(SceneState& scene_state) override;

    // Update
    void update(SceneState& scene_state) override;

private:
    // LOD levels
    std::shared_ptr<ConicSurface> lod_[3];

    Point3 world_pos_;
    std::string name_;
    int last_lod_ = -1;

    static constexpr float DIST_HIGH = 60.0f;
    static constexpr float DIST_MEDIUM = 150.0f;
};

} // namespace cg
#endif
