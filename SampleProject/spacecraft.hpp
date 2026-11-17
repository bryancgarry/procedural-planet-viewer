//============================================================================
//  spacecraft.hpp
//  JHU_605.767 Final Project
//
//  Manages spacecraft state — position, orientation, and velocity — for a
//  6DOF style controls (pitch/yaw/roll + thrust/strafe).
//
//  Collision detection uses a two-phase bounding sphere approach:
//    Phase 1 — coarse: ship vs body's maximum terrain bounding sphere
//    Phase 2 — fine:   ship vs body's collision_radius (tuned above average terrain)
//
//  Gravity uses an inverse-square law clamped to each body's sphere of
//  influence radius, so the planet and moon have independent gravitational
//  domains that don't overlap.
//============================================================================

#ifndef __SPACECRAFT_HPP__
#define __SPACECRAFT_HPP__

#include "geometry/point3.hpp"
#include "geometry/vector3.hpp"

namespace cg
{

/**
 * Parameters for a celestial body used in gravity and collision.
 * collision_radius is set slightly above average terrain height.
 * soi_radius defines how far out gravity reaches.
 */
struct CelestialBody
{
    Point3 position;        // world space center (planet = origin)
    float  radius;          // base sphere radius
    float  collision_radius;// bounding sphere for collision — above avg terrain
    float  soi_radius;      // sphere of influence radius for gravity
};

class Spacecraft
{
  public:
    Spacecraft();

    /**
     * Update physics each frame — apply gravity, drag, integrate velocity,
     * and resolve collisions against both bodies.
     * @param  dt      Delta time in seconds.
     * @param  planet  Planet collision and gravity parameters.
     * @param  moon    Moon collision and gravity parameters.
     */
    void update(float dt,
                const CelestialBody &planet,
                const CelestialBody &moon);

    // --- KSP-style controls ---
    void thrust(float amount);       // I/K — forward/backward
    void strafe_right(float amount); // J/L — left/right
    void strafe_up(float amount);    // U/O — up/down
    void pitch(float degrees);       // W/S — pitch up/down
    void yaw(float degrees);         // A/D — yaw left/right
    void roll(float degrees);        // Q/E — roll left/right

    // --- State accessors ---
    const Point3  &get_position() const { return position_; }
    const Vector3 &get_forward()  const { return forward_;  }
    const Vector3 &get_up()       const { return up_;       }
    const Vector3 &get_right()    const { return right_;    }

    // Camera positions — updated each frame by update_spacecraft_camera()
    Point3 get_third_person_camera_pos() const;
    Point3 get_first_person_camera_pos() const;
    Point3 get_look_at_point()           const;

    // Visual and collision radius — both use this value so the ship
    // sits flush with the terrain boundary it collides against.
    static constexpr float SHIP_RADIUS = 0.3f;

  private:
    Point3  position_;
    Vector3 forward_;
    Vector3 up_;
    Vector3 right_;
    Vector3 velocity_;

    /**
     * Re-orthogonalize the orientation frame after each rotation to prevent
     * accumulated floating-point drift from corrupting the basis vectors.
     * Uses forward as the primary axis and recomputes right and up from it.
     */
    void orthogonalize();

    /**
     * Resolve collision of new_pos against one body.
     * Pushes the ship back to the collision boundary and cancels inward velocity.
     * @return  True if a collision occurred.
     */
    bool resolve_collision(Point3 &new_pos, const CelestialBody &body);

    /**
     * Apply gravitational acceleration toward a body, using inverse-square
     * law scaled to the body's radius. Only fires within soi_radius.
     */
    void apply_gravity(float dt, const CelestialBody &body);
};

} // namespace cg

#endif
