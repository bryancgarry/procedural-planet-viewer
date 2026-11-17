//============================================================================
//  cloud_shader_node.hpp
//  JHU_605.767 Final Project
//
//  Shader node for the animated cloud layer. Manages the cloud.vert and
//  cloud.frag shader program. Enables alpha blending and disables depth
//  write during draw so clouds composite transparently over the terrain
//  and water without occluding the atmosphere shell behind them.
//  The time uniform is updated each frame to animate cloud drift.
//============================================================================

#ifndef __CLOUD_SHADER_NODE_HPP__
#define __CLOUD_SHADER_NODE_HPP__

#include "scene/shader_node.hpp"
#include "geometry/vector3.hpp"
#include "scene/color4.hpp"

#include <array>

namespace cg
{

class CloudShaderNode : public ShaderNode
{
  public:
    bool get_locations() override;

    /**
     * Activate the shader, push camera position and time, enable alpha
     * blending with depth mask off, draw children, restore state.
     */
    void draw(SceneState &scene_state) override;

    void set_global_ambient(const Color4 &global_ambient);

    /**
     * Set cloud appearance parameters. Call once at startup.
     * @param  sun_dir      Normalized direction toward sun (g_sun_direction).
     * @param  threshold    Noise value above which clouds appear (0.05-0.3).
     * @param  softness     Edge feathering width (0.1-0.2).
     * @param  opacity      Maximum cloud alpha (0.7-1.0).
     * @param  drift_dir    Normalized wind direction in sphere space.
     * @param  drift_speed  Cloud drift speed (0.01-0.05).
     */
    void set_cloud_params(const Vector3 &sun_dir,
                          float          threshold,
                          float          softness,
                          float          opacity,
                          const Vector3 &drift_dir,
                          float          drift_speed);

    /**
     * Update the animation time uniform. Call every frame with g_time.
     */
    void set_time(float time);

    int32_t get_position_loc() const;
    int32_t get_normal_loc()   const;

  protected:
    GLint position_loc_;
    GLint normal_loc_;

    GLint pvm_matrix_loc_;
    GLint model_matrix_loc_;
    GLint normal_matrix_loc_;
    GLint camera_position_loc_;

    GLint   global_ambient_loc_;
    GLint   light_count_loc_;
    int32_t light_count_;
    std::array<LightUniforms, 3> lights_;

    GLint sun_direction_loc_;
    GLint time_loc_;
    GLint cloud_threshold_loc_;
    GLint cloud_softness_loc_;
    GLint cloud_opacity_loc_;
    GLint drift_direction_loc_;
    GLint drift_speed_loc_;
};

} // namespace cg

#endif
