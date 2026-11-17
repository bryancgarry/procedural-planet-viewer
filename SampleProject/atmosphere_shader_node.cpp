//============================================================================
//  atmosphere_shader_node.cpp
//  JHU_605.767 Final Project
//============================================================================

#include "SampleProject/atmosphere_shader_node.hpp"
#include <iostream>

namespace cg
{

bool AtmosphereShaderNode::get_locations()
{
    GLuint prog = shader_program_.get_program();

    position_loc_ = glGetAttribLocation(prog, "vtx_position");
    if (position_loc_ < 0) {
        std::cout << "AtmosphereShaderNode: Error getting vtx_position\n";
        return false;
    }
    normal_loc_ = glGetAttribLocation(prog, "vtx_normal");
    if (normal_loc_ < 0) {
        std::cout << "AtmosphereShaderNode: Error getting vtx_normal\n";
        return false;
    }

    // Matrix uniforms — needed for the sphere shell geometry transform
    pvm_matrix_loc_    = glGetUniformLocation(prog, "pvm_matrix");
    model_matrix_loc_  = glGetUniformLocation(prog, "model_matrix");
    normal_matrix_loc_ = glGetUniformLocation(prog, "normal_matrix");

    // Per-frame — pushed in draw() from scene_state each frame
    camera_position_loc_ = glGetUniformLocation(prog, "camera_position");

    // Static — set once via set_params()
    planet_radius_loc_         = glGetUniformLocation(prog, "planet_radius");
    atmosphere_radius_loc_     = glGetUniformLocation(prog, "atmosphere_radius");
    sun_direction_loc_         = glGetUniformLocation(prog, "sun_direction");
    sun_intensity_loc_         = glGetUniformLocation(prog, "sun_intensity");
    rayleigh_coeffs_loc_       = glGetUniformLocation(prog, "rayleigh_coeffs");
    mie_coeff_loc_             = glGetUniformLocation(prog, "mie_coeff");
    rayleigh_scale_height_loc_ = glGetUniformLocation(prog, "rayleigh_scale_height");
    mie_scale_height_loc_      = glGetUniformLocation(prog, "mie_scale_height");

    return true;
}

void AtmosphereShaderNode::draw(SceneState &scene_state)
{
    shader_program_.use();

    // Set standard matrix locations so TransformNode writes to this shader
    scene_state.position_loc        = position_loc_;
    scene_state.normal_loc          = normal_loc_;
    scene_state.texture_loc         = -1;
    scene_state.pvm_matrix_loc      = pvm_matrix_loc_;
    scene_state.model_matrix_loc    = model_matrix_loc_;
    scene_state.normal_matrix_loc   = normal_matrix_loc_;
    scene_state.camera_position_loc = camera_position_loc_;
    scene_state.material_ambient_loc   = -1;
    scene_state.material_diffuse_loc   = -1;
    scene_state.material_specular_loc  = -1;
    scene_state.material_emission_loc  = -1;
    scene_state.material_shininess_loc = -1;
    scene_state.use_texture_loc        = -1;
    scene_state.texture_unit_loc       = -1;
    scene_state.lightcount_loc         = -1;  // atmosphere has no light uniforms

    // Camera ran before this shader activated — push its position directly
    glUniform3fv(camera_position_loc_, 1, &scene_state.camera_position.x);

    // Alpha blending for transparent atmosphere
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Disable culling — both inner and outer shell faces contribute glow
    // when the camera is inside the atmosphere (flying near the surface).
    glDisable(GL_CULL_FACE);

    // Keep depth TEST on so planet/moon/clouds occlude the atmosphere,
    // but disable depth WRITE so the shell doesn't occlude objects behind it.
    glDepthMask(GL_FALSE);

    SceneNode::draw(scene_state);

    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glDisable(GL_BLEND);
}

void AtmosphereShaderNode::set_params(float planet_r, float atmo_r,
                                       const Vector3 &sun_dir, float sun_i)
{
    shader_program_.use();
    glUniform1f(planet_radius_loc_,     planet_r);
    glUniform1f(atmosphere_radius_loc_, atmo_r);
    glUniform3fv(sun_direction_loc_,    1, &sun_dir.x);
    glUniform1f(sun_intensity_loc_,     sun_i);

    // Rayleigh coefficients — blue channel (33.1) much higher than red (1.5)
    // and green (6.0) to bias scattering toward blue, simulating Earth's sky.
    // Mie near zero to suppress the brown/orange tint it produces.
    glUniform3f(rayleigh_coeffs_loc_,       1.5f, 6.0f, 33.1f);
    glUniform1f(mie_coeff_loc_,             0.0001f);
    glUniform1f(rayleigh_scale_height_loc_, 3.0f);
    glUniform1f(mie_scale_height_loc_,      0.3f);
}

void AtmosphereShaderNode::set_sun_direction(const Vector3 &sun_dir)
{
    shader_program_.use();
    glUniform3fv(sun_direction_loc_, 1, &sun_dir.x);
}

int32_t AtmosphereShaderNode::get_position_loc() const { return position_loc_; }
int32_t AtmosphereShaderNode::get_normal_loc()   const { return normal_loc_;   }

} // namespace cg
