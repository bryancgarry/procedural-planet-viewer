//============================================================================
//  atmosphere_shader_node.hpp
//  JHU_605.767 Final Project
//
//  Shader node for the ray-marched atmosphere. Renders the atmosphere as an
//  icosphere shell using atmosphere.vert and atmosphere.frag. The shell is
//  drawn with depth testing enabled but depth writes disabled, so the planet,
//  moon, and clouds correctly occlude it via the depth buffer without the
//  atmosphere needing any explicit occlusion logic.
//
//  Back-face culling is disabled during draw so both the inner and outer
//  faces of the shell contribute glow when the camera is inside the atmosphere.
//============================================================================

#ifndef __ATMOSPHERE_SHADER_NODE_HPP__
#define __ATMOSPHERE_SHADER_NODE_HPP__

#include "scene/shader_node.hpp"
#include "geometry/vector3.hpp"
#include "geometry/point3.hpp"

namespace cg
{

class AtmosphereShaderNode : public ShaderNode
{
  public:
    bool get_locations() override;

    /**
     * Activate the shader, push camera position, enable alpha blending
     * and disable culling/depth-write, draw children, restore state.
     */
    void draw(SceneState &scene_state) override;

    /**
     * Set atmosphere parameters. Call once at startup after create().
     * @param  planet_r  Density falloff anchor radius (not visual surface).
     * @param  atmo_r    Shell radius — must match the geometry scale transform.
     * @param  sun_dir   Normalized direction FROM planet TOWARD sun.
     * @param  sun_i     Overall brightness multiplier.
     */
    void set_params(float planet_r, float atmo_r,
                    const Vector3 &sun_dir, float sun_i);

    void set_sun_direction(const Vector3 &sun_dir);

    int32_t get_position_loc() const;
    int32_t get_normal_loc()   const;

  private:
    GLint position_loc_;
    GLint normal_loc_;

    GLint pvm_matrix_loc_;
    GLint model_matrix_loc_;
    GLint normal_matrix_loc_;
    GLint camera_position_loc_;

    GLint planet_radius_loc_;
    GLint atmosphere_radius_loc_;
    GLint sun_direction_loc_;
    GLint sun_intensity_loc_;
    GLint rayleigh_coeffs_loc_;
    GLint mie_coeff_loc_;
    GLint rayleigh_scale_height_loc_;
    GLint mie_scale_height_loc_;
};

} // namespace cg
#endif
