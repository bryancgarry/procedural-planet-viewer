//============================================================================
//  spacecraft.cpp
//  JHU_605.767 Final Project
//============================================================================

#include "SampleProject/spacecraft.hpp"
#include "geometry/geometry.hpp"
#include <cmath>

namespace cg
{

Spacecraft::Spacecraft()
    : position_(0.0f, -80.0f, 0.0f)
    , forward_(0.0f,  1.0f,  0.0f)
    , up_(     0.0f,  0.0f,  1.0f)
    , right_(  1.0f,  0.0f,  0.0f)
    , velocity_(0.0f, 0.0f,  0.0f)
{
}

// ---------------------------------------------------------------------------
// Controls
// ---------------------------------------------------------------------------

void Spacecraft::thrust(float amount)       { velocity_ += forward_ * amount; }
void Spacecraft::strafe_right(float amount) { velocity_ += right_   * amount; }
void Spacecraft::strafe_up(float amount)    { velocity_ += up_      * amount; }

void Spacecraft::pitch(float degrees)
{
    float rad = degrees_to_radians(degrees);
    float c   = std::cos(rad);
    float s   = std::sin(rad);

    // Rotate forward and up around the right axis — right unchanged
    Vector3 new_fwd = forward_ * c + up_ * s;
    Vector3 new_up  = up_      * c - forward_ * s;
    forward_ = new_fwd;
    up_      = new_up;
    orthogonalize();
}

void Spacecraft::yaw(float degrees)
{
    float rad = degrees_to_radians(degrees);
    float c   = std::cos(rad);
    float s   = std::sin(rad);

    // Rotate forward and right around the up axis — up unchanged
    Vector3 new_fwd = forward_ * c - right_ * s;
    Vector3 new_rt  = right_   * c + forward_ * s;
    forward_ = new_fwd;
    right_   = new_rt;
    orthogonalize();
}

void Spacecraft::roll(float degrees)
{
    float rad = degrees_to_radians(degrees);
    float c   = std::cos(rad);
    float s   = std::sin(rad);

    // Rotate right and up around the forward axis — forward unchanged
    Vector3 new_rt = right_ * c + up_    * s;
    Vector3 new_up = up_    * c - right_ * s;
    right_ = new_rt;
    up_    = new_up;
    orthogonalize();
}

void Spacecraft::orthogonalize()
{
    // Forward is the primary axis — normalize it first.
    // Right is recomputed perpendicular to forward and up,
    // then up is recomputed to complete the orthonormal frame.
    // This order ensures roll is preserved across pitch and yaw operations.
    forward_.normalize();
    right_ = forward_.cross(up_);
    right_.normalize();
    up_ = right_.cross(forward_);
    up_.normalize();
}

// ---------------------------------------------------------------------------
// Camera positions
// ---------------------------------------------------------------------------

Point3 Spacecraft::get_third_person_camera_pos() const
{
    // Offset behind and slightly above in ship local space
    return position_ + (forward_ * -4.0f) + (up_ * 1.0f);
}

Point3 Spacecraft::get_first_person_camera_pos() const
{
    return position_ + forward_ * 1.0f;
}

Point3 Spacecraft::get_look_at_point() const
{
    return position_ + forward_ * 10.0f;
}

// ---------------------------------------------------------------------------
// Physics
// ---------------------------------------------------------------------------

void Spacecraft::apply_gravity(float dt, const CelestialBody &body)
{
    constexpr float GRAVITY_STRENGTH = 8.0f;

    Vector3 to_body(body.position.x - position_.x,
                    body.position.y - position_.y,
                    body.position.z - position_.z);

    float dist = to_body.norm();

    // Only apply gravity within this body's sphere of influence
    if (dist < 0.01f || dist > body.soi_radius) return;

    to_body.normalize();

    // Inverse-square law — gravity is strongest at the surface
    float gravity_scale = (body.radius * body.radius) / (dist * dist);
    velocity_ += to_body * GRAVITY_STRENGTH * gravity_scale * dt;
}

bool Spacecraft::resolve_collision(Point3 &new_pos, const CelestialBody &body)
{
    Vector3 to_ship(new_pos.x - body.position.x,
                    new_pos.y - body.position.y,
                    new_pos.z - body.position.z);

    float dist = to_ship.norm();
    if (dist < 0.001f) return false;

    // Bounding sphere collision — collision_radius is tuned to sit slightly
    // above the average terrain height of the body.
    float min_dist = body.collision_radius + SHIP_RADIUS;
    if (dist >= min_dist) return false;

    Vector3 dir = to_ship * (1.0f / dist);

    // Push ship back to the collision boundary
    new_pos = Point3(body.position.x + dir.x * min_dist,
                     body.position.y + dir.y * min_dist,
                     body.position.z + dir.z * min_dist);

    // Cancel the inward velocity component so the ship doesn't tunnel through
    float v_inward = -(velocity_.dot(dir));
    if (v_inward > 0.0f)
        velocity_ += dir * v_inward;

    return true;
}

void Spacecraft::update(float dt,
                         const CelestialBody &planet,
                         const CelestialBody &moon)
{
    apply_gravity(dt, planet);
    apply_gravity(dt, moon);

    // Gentle drag — velocity persists in space but bleeds off slowly
    velocity_ *= 0.995f;

    Point3 new_pos = position_ + velocity_ * dt;
    resolve_collision(new_pos, planet);
    resolve_collision(new_pos, moon);
    position_ = new_pos;
}

} // namespace cg
