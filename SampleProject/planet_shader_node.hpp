//============================================================================
//  planet_shader_node.hpp
//  JHU_605.767 Final Project
//
//  Shader node for the procedural planet and moon. Manages the planet.vert
//  and planet.frag shader program, populating scene state with all uniform
//  and attribute locations so the standard scene graph nodes (TransformNode,
//  PresentationNode, LightNode) write to the correct program uniforms.
//============================================================================

#ifndef __PLANET_SHADER_NODE_HPP__
#define __PLANET_SHADER_NODE_HPP__

#include "scene/shader_node.hpp"
#include "scene/color4.hpp"

#include <array>

namespace cg
{

class PlanetShaderNode : public ShaderNode
{
  public:
    /**
     * Query all uniform and attribute locations from the compiled shaders.
     * Must be called after create() succeeds.
     * @return  True if all critical locations were found.
     */
    bool get_locations() override;

    /**
     * Activate the shader and populate scene state with uniform/attribute
     * locations so child nodes write to the correct program.
     */
    void draw(SceneState &scene_state) override;

    /**
     * Set the global ambient lighting intensity.
     * @param  global_ambient  RGBA ambient color.
     */
    void set_global_ambient(const Color4 &global_ambient);

    /**
     * Set noise terrain parameters. Values must match those passed to
     * PlanetCollisionMesh::capture() if collision is used.
     * @param  amplitude  Vertical scale of terrain features (fraction of radius).
     * @param  frequency  Horizontal scale — higher = more zoomed-in detail.
     */
    void set_noise_params(float amplitude, float frequency);

    /**
     * Toggle elevation-based terrain coloring in the fragment shader.
     * @param  enabled  True = elevation color bands (planet mode).
     *                  False = material_diffuse color (moon mode).
     */
    void set_elevation_color(bool enabled);

    int32_t get_position_loc() const;
    int32_t get_normal_loc()   const;

    // Expose the compiled program for transform feedback capture
    GLuint get_program() const { return shader_program_.get_program(); }

  protected:
    GLint position_loc_;
    GLint normal_loc_;

    GLint pvm_matrix_loc_;
    GLint model_matrix_loc_;
    GLint normal_matrix_loc_;
    GLint camera_position_loc_;

    GLint material_ambient_loc_;
    GLint material_diffuse_loc_;
    GLint material_specular_loc_;
    GLint material_emission_loc_;
    GLint material_shininess_loc_;

    int32_t                      light_count_;
    GLint                        light_count_loc_;
    GLint                        global_ambient_loc_;
    std::array<LightUniforms, 3> lights_;

    GLint noise_amplitude_loc_;
    GLint noise_frequency_loc_;
    GLint use_elevation_color_loc_;
};

} // namespace cg

#endif
