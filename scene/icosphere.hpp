//============================================================================
//  icosphere.hpp
//  JHU_605.767 Final Project
//
//  Subdivided icosphere geometry node. Builds a geodesic sphere by
//  recursively subdividing the 20 faces of a regular icosahedron, then
//  projecting each midpoint back onto the sphere surface. The resulting
//  mesh has nearly uniform triangle sizes, making it well-suited for
//  vertex shader displacement (planet terrain) and smooth sphere shells
//  (atmosphere, clouds, water).
//
//  On a unit icosphere, vtx_position == vtx_normal (both are the unit
//  radial direction), which the planet vertex shader exploits for terrain
//  displacement along the normal direction.
//============================================================================

#ifndef __SCENE_ICOSPHERE_NODE_HPP__
#define __SCENE_ICOSPHERE_NODE_HPP__

#include "scene/geometry_node.hpp"
#include "geometry/types.hpp"

#include <map>
#include <utility>
#include <vector>

namespace cg
{

class IcosphereNode : public GeometryNode
{
  public:
    /**
     * Build and upload an icosphere to the GPU.
     * @param  subdivisions  Subdivision passes — each pass multiplies triangle
     *                       count by 4. Recommended: 5 (shells), 6 (planet).
     * @param  radius        Sphere radius on the unit sphere (use 1.0, scale
     *                       to world size via TransformNode).
     * @param  position_loc  Vertex position attribute location from shader.
     * @param  normal_loc    Vertex normal attribute location from shader.
     */
    IcosphereNode(int     subdivisions,
                  float   radius,
                  int32_t position_loc,
                  int32_t normal_loc);

    ~IcosphereNode();

    void draw(SceneState &scene_state) override;

    float get_radius() const { return radius_; }

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
