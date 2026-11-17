//============================================================================
//  water_shader_node.hpp
//  JHU_605.767 Final Project
//
//  Shader node for the ocean water surface. Manages the water.vert and
//  water.frag shader program, populating scene state with all uniform and
//  attribute locations. Enables alpha blending during draw so shoreline
//  transparency composites correctly over the terrain.
//============================================================================

#ifndef __WATER_SHADER_NODE_HPP__
#define __WATER_SHADER_NODE_HPP__

#include "scene/shader_node.hpp"
#include "scene/color4.hpp"
#include "geometry/vector3.hpp"

#include <array>

namespace cg
{

class WaterShaderNode : public ShaderNode
{
  public:
    bool get_locations() override;

    /**
     * Activate the shader, push camera position, enable alpha blending,
     * draw children, then restore blend state.
     */
    void draw(SceneState &scene_state) override;

    void set_global_ambient(const Color4 &global_ambient);

    /**
     * Set water appearance parameters. Call once at startup.
     * @param  sun_dir          Normalized direction toward sun (g_sun_direction).
     * @param  shallow_color    Water color near shorelines.
     * @param  deep_color       Water color in deep ocean.
     * @param  shininess        Specular exponent — higher = tighter sun glints.
     * @param  sky_color        Color reflected at grazing angles (Fresnel).
     * @param  fresnel_strength Strength of Fresnel edge reflection (0-1).
     */
    void set_water_params(const Vector3 &sun_dir,
                          const Color4  &shallow_color,
                          const Color4  &deep_color,
                          float          shininess,
                          const Vector3 &sky_color,
                          float          fresnel_strength);

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
    GLint water_shallow_color_loc_;
    GLint water_deep_color_loc_;
    GLint water_shininess_loc_;
    GLint sky_color_loc_;
    GLint fresnel_strength_loc_;
};

} // namespace cg

#endif
