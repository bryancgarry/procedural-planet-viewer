#include "scene/view_frustum.hpp"
#include <cmath>

namespace cg
{

ViewFrustum::ViewFrustum() {}

void ViewFrustum::construct(const Matrix4x4& projection, const Matrix4x4& view)
{
    Matrix4x4 pv = projection * view;

    auto R = [&](int row, int col) { return pv.m(row, col); };

    auto make_plane = [&](int sign, int row) -> Plane
    {
        float a = R(3,0) + sign * R(row,0);
        float b = R(3,1) + sign * R(row,1);
        float c = R(3,2) + sign * R(row,2);
        float w = R(3,3) + sign * R(row,3);

        float len = std::sqrt(a*a + b*b + c*c);
        if (len > 0.0f) { a /= len; b /= len; c /= len; w /= len; }

        Plane p;
        p.a =  a;
        p.b =  b;
        p.c =  c;
        p.d = -w;
        return p;
    };

    planes_[0] = make_plane(+1, 0);   // left
    planes_[1] = make_plane(-1, 0);   // right
    planes_[2] = make_plane(+1, 1);   // bottom
    planes_[3] = make_plane(-1, 1);   // top
    planes_[4] = make_plane(+1, 2);   // near
    planes_[5] = make_plane(-1, 2);   // far
}

FrustumIntersectType ViewFrustum::intersect(const BoundingSphere& sphere) const
{
    // Plane::solve returns positive on the inside (normal-facing) side.
    // dist + radius < 0: sphere fully outside this plane
    // dist - radius < 0: sphere straddles this plane
    bool all_inside = true;

    for (const auto& p : planes_)
    {
        float dist = p.solve(sphere.center);

        if (dist + sphere.radius < 0.0f)
            return FrustumIntersectType::OUTSIDE;
        if (dist - sphere.radius < 0.0f)
            all_inside = false;
    }

    return all_inside ? FrustumIntersectType::INSIDE
                      : FrustumIntersectType::INTERSECT;
}

FrustumIntersectType ViewFrustum::intersect(const AABB& box) const
{
    // s = signed distance of AABB center to plane (positive = inside)
    // e = projected half-extent of AABB onto plane normal
    // s + e < 0: AABB fully outside this plane -- cull
    // s - e < 0: AABB straddles this plane -- not fully inside
    float cx = 0.5f * (box.max.x + box.min.x);
    float cy = 0.5f * (box.max.y + box.min.y);
    float cz = 0.5f * (box.max.z + box.min.z);
    float hx = 0.5f * (box.max.x - box.min.x);
    float hy = 0.5f * (box.max.y - box.min.y);
    float hz = 0.5f * (box.max.z - box.min.z);

    bool all_inside = true;

    for (const auto& p : planes_)
    {
        float s = p.solve(Point3(cx, cy, cz));
        float e = hx * std::abs(p.a) + hy * std::abs(p.b) + hz * std::abs(p.c);

        if (s + e < 0.0f)
            return FrustumIntersectType::OUTSIDE;
        if (s - e < 0.0f)
            all_inside = false;
    }

    return all_inside ? FrustumIntersectType::INSIDE
                      : FrustumIntersectType::INTERSECT;
}

} // namespace cg
