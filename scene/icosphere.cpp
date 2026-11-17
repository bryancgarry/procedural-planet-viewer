//============================================================================
//  icosphere.cpp
//  JHU_605.767 Final Project
//============================================================================

#include "scene/icosphere.hpp"

#include <cmath>

namespace cg
{

IcosphereNode::IcosphereNode(int subdivisions, float radius,
                             int32_t position_loc, int32_t normal_loc)
    : radius_(radius), face_count_(0), vao_(0), vbo_(0), facebuffer_(0)
{
    node_type_ = SceneNodeType::GEOMETRY;
    build(subdivisions);
    create_vertex_buffers(position_loc, normal_loc);
}

IcosphereNode::~IcosphereNode()
{
    glDeleteVertexArrays(1, &vao_);
    glDeleteBuffers(1, &vbo_);
    glDeleteBuffers(1, &facebuffer_);
}

// Add a vertex to the vertex list, normalizing onto the sphere surface
// then scaling by radius. On a unit icosphere, position == normal.
static int add_sphere_vertex(std::vector<VertexAndNormal> &verts,
                              float x, float y, float z, float radius)
{
    Vector3 n(x, y, z);
    n.normalize();
    VertexAndNormal v;
    v.vertex = Point3(n.x * radius, n.y * radius, n.z * radius);
    v.normal = n;
    verts.push_back(v);
    return (int)verts.size() - 1;
}

void IcosphereNode::build(int subdivisions)
{
    // Golden ratio — defines the 12 vertices of a regular icosahedron
    const float t = (1.0f + std::sqrt(5.0f)) / 2.0f;

    add_sphere_vertex(vertices_, -1,  t,  0, radius_);
    add_sphere_vertex(vertices_,  1,  t,  0, radius_);
    add_sphere_vertex(vertices_, -1, -t,  0, radius_);
    add_sphere_vertex(vertices_,  1, -t,  0, radius_);
    add_sphere_vertex(vertices_,  0, -1,  t, radius_);
    add_sphere_vertex(vertices_,  0,  1,  t, radius_);
    add_sphere_vertex(vertices_,  0, -1, -t, radius_);
    add_sphere_vertex(vertices_,  0,  1, -t, radius_);
    add_sphere_vertex(vertices_,  t,  0, -1, radius_);
    add_sphere_vertex(vertices_,  t,  0,  1, radius_);
    add_sphere_vertex(vertices_, -t,  0, -1, radius_);
    add_sphere_vertex(vertices_, -t,  0,  1, radius_);

    // 20 base triangles — CCW winding
    std::vector<std::array<int,3>> tris = {
        {0,11,5},{0,5,1},{0,1,7},{0,7,10},{0,10,11},
        {1,5,9},{5,11,4},{11,10,2},{10,7,6},{7,1,8},
        {3,9,4},{3,4,2},{3,2,6},{3,6,8},{3,8,9},
        {4,9,5},{2,4,11},{6,2,10},{8,6,7},{9,8,1}
    };

    // Subdivide — each pass splits every triangle into 4 smaller triangles
    // and projects midpoints back onto the sphere surface.
    std::map<std::pair<int,int>, int> cache;
    for (int i = 0; i < subdivisions; i++) {
        std::vector<std::array<int,3>> next;
        next.reserve(tris.size() * 4);
        for (auto &tri : tris) {
            int a = get_midpoint(tri[0], tri[1], cache);
            int b = get_midpoint(tri[1], tri[2], cache);
            int c = get_midpoint(tri[2], tri[0], cache);
            next.push_back({tri[0], a, c});
            next.push_back({tri[1], b, a});
            next.push_back({tri[2], c, b});
            next.push_back({a, b, c});
        }
        tris = std::move(next);
    }

    faces_.reserve(tris.size() * 3);
    for (auto &tri : tris) {
        faces_.push_back((uint16_t)tri[0]);
        faces_.push_back((uint16_t)tri[1]);
        faces_.push_back((uint16_t)tri[2]);
    }
    face_count_ = (GLsizei)faces_.size();
}

int IcosphereNode::get_midpoint(int a, int b,
                                 std::map<std::pair<int,int>, int> &cache)
{
    // Use sorted pair as key so (a,b) and (b,a) map to the same midpoint
    auto key = std::make_pair(std::min(a, b), std::max(a, b));
    auto it  = cache.find(key);
    if (it != cache.end()) return it->second;

    const Point3 &pa = vertices_[a].vertex;
    const Point3 &pb = vertices_[b].vertex;
    int idx = add_sphere_vertex(vertices_,
        (pa.x + pb.x) * 0.5f,
        (pa.y + pb.y) * 0.5f,
        (pa.z + pb.z) * 0.5f,
        radius_);
    cache[key] = idx;
    return idx;
}

void IcosphereNode::create_vertex_buffers(int32_t position_loc,
                                           int32_t normal_loc)
{
    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);

    glGenBuffers(1, &vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER,
                 vertices_.size() * sizeof(VertexAndNormal),
                 vertices_.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(position_loc, 3, GL_FLOAT, GL_FALSE,
                          sizeof(VertexAndNormal),
                          (void*)offsetof(VertexAndNormal, vertex));
    glEnableVertexAttribArray(position_loc);

    glVertexAttribPointer(normal_loc, 3, GL_FLOAT, GL_FALSE,
                          sizeof(VertexAndNormal),
                          (void*)offsetof(VertexAndNormal, normal));
    glEnableVertexAttribArray(normal_loc);

    glGenBuffers(1, &facebuffer_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, facebuffer_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 faces_.size() * sizeof(uint16_t),
                 faces_.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);
}

void IcosphereNode::draw(SceneState &scene_state)
{
    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, face_count_, GL_UNSIGNED_SHORT, 0);
    glBindVertexArray(0);
}

} // namespace cg
