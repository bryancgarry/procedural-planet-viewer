//============================================================================
//  water_node.hpp
//  JHU_605.767 Final Project
//
//  Ocean water geometry node. Builds a smooth icosphere at sea level
//  (radius = 1.0 on the unit sphere, world radius 30 after scale transform).
//  No noise displacement is applied — the water surface stays perfectly
//  smooth and the terrain depth buffer naturally occludes land that rises
//  above sea level.
//
//  Structurally identical to IcosphereNode but kept as a separate class
//  since it uses different shaders thought it might diverge in at some point.
//============================================================================

#ifndef __WATER_NODE_HPP__
#define __WATER_NODE_HPP__

#include "scene/geometry_node.hpp"
#include "geometry/types.hpp"

#include <map>
#include <utility>
#include <vector>

namespace cg
{

class WaterNode : public GeometryNode
{
  public:
    /**
     * Build and upload a smooth icosphere to the GPU.
     * @param  subdivisions  Should match the planet icosphere subdivision
     *                       level so mesh density is consistent.
     * @param  radius        Unit sphere radius (use 1.0, scale via TransformNode).
     * @param  position_loc  Vertex position attribute location from shader.
     * @param  normal_loc    Vertex normal attribute location from shader.
     */
    WaterNode(int     subdivisions,
              float   radius,
              int32_t position_loc,
              int32_t normal_loc);

    ~WaterNode();

    void draw(SceneState &scene_state) override;

  private:
    float   radius_;
    GLsizei face_count_;
    GLuint  vao_;
    GLuint  vbo_;
    GLuint  facebuffer_;

    std::vector<VertexAndNormal> vertices_;
    std::vector<uint16_t>        faces_;

    void build(int subdivisions);
    int  get_midpoint(int a, int b, std::map<std::pair<int,int>, int> &cache);
    void create_vertex_buffers(int32_t position_loc, int32_t normal_loc);
};

} // namespace cg

#endif
