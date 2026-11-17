//============================================================================
//  planet_shader_node.cpp
//  JHU_605.767 Final Project
//============================================================================

#include "SampleProject/planet_shader_node.hpp"

#include <cstdio>
#include <iostream>

namespace cg
{

bool PlanetShaderNode::get_locations()
{
    GLuint prog = shader_program_.get_program();

    position_loc_ = glGetAttribLocation(prog, "vtx_position");
    if (position_loc_ < 0) {
        std::cout << "PlanetShaderNode: Error getting vtx_position location\n";
        return false;
    }
    normal_loc_ = glGetAttribLocation(prog, "vtx_normal");
    if (normal_loc_ < 0) {
        std::cout << "PlanetShaderNode: Error getting vtx_normal location\n";
        return false;
    }

    pvm_matrix_loc_ = glGetUniformLocation(prog, "pvm_matrix");
    if (pvm_matrix_loc_ < 0) {
        std::cout << "PlanetShaderNode: Error getting pvm_matrix location\n";
        return false;
    }
    model_matrix_loc_ = glGetUniformLocation(prog, "model_matrix");
    if (model_matrix_loc_ < 0) {
        std::cout << "PlanetShaderNode: Error getting model_matrix location\n";
        return false;
    }
    normal_matrix_loc_ = glGetUniformLocation(prog, "normal_matrix");
    if (normal_matrix_loc_ < 0) {
        std::cout << "PlanetShaderNode: Error getting normal_matrix location\n";
        return false;
    }

    camera_position_loc_ = glGetUniformLocation(prog, "camera_position");

    material_ambient_loc_   = glGetUniformLocation(prog, "material_ambient");
    material_diffuse_loc_   = glGetUniformLocation(prog, "material_diffuse");
    material_specular_loc_  = glGetUniformLocation(prog, "material_specular");
    material_emission_loc_  = glGetUniformLocation(prog, "material_emission");
    material_shininess_loc_ = glGetUniformLocation(prog, "material_shininess");

    light_count_        = 3;
    light_count_loc_    = glGetUniformLocation(prog, "num_lights");
    global_ambient_loc_ = glGetUniformLocation(prog, "global_light_ambient");

    char name[128];
    for (int i = 0; i < light_count_; i++) {
        snprintf(name, 128, "lights[%d].enabled",               i);
        lights_[i].enabled        = glGetUniformLocation(prog, name);
        snprintf(name, 128, "lights[%d].spotlight",             i);
        lights_[i].spotlight      = glGetUniformLocation(prog, name);
        snprintf(name, 128, "lights[%d].position",              i);
        lights_[i].position       = glGetUniformLocation(prog, name);
        snprintf(name, 128, "lights[%d].ambient",               i);
        lights_[i].ambient        = glGetUniformLocation(prog, name);
        snprintf(name, 128, "lights[%d].diffuse",               i);
        lights_[i].diffuse        = glGetUniformLocation(prog, name);
        snprintf(name, 128, "lights[%d].specular",              i);
        lights_[i].specular       = glGetUniformLocation(prog, name);
        snprintf(name, 128, "lights[%d].constant_attenuation",  i);
        lights_[i].att_constant   = glGetUniformLocation(prog, name);
        snprintf(name, 128, "lights[%d].linear_attenuation",    i);
        lights_[i].att_linear     = glGetUniformLocation(prog, name);
        snprintf(name, 128, "lights[%d].quadratic_attenuation", i);
        lights_[i].att_quadratic  = glGetUniformLocation(prog, name);
        snprintf(name, 128, "lights[%d].spot_cutoff",           i);
        lights_[i].spot_cutoff    = glGetUniformLocation(prog, name);
        snprintf(name, 128, "lights[%d].spot_exponent",         i);
        lights_[i].spot_exponent  = glGetUniformLocation(prog, name);
        snprintf(name, 128, "lights[%d].spot_direction",        i);
        lights_[i].spot_direction = glGetUniformLocation(prog, name);
    }

    noise_amplitude_loc_     = glGetUniformLocation(prog, "noise_amplitude");
    noise_frequency_loc_     = glGetUniformLocation(prog, "noise_frequency");
    use_elevation_color_loc_ = glGetUniformLocation(prog, "use_elevation_color");

    return true;
}

void PlanetShaderNode::draw(SceneState &scene_state)
{
    shader_program_.use();

    // Populate scene state with this shader's locations so that
    // TransformNode, PresentationNode, and LightNode all write
    // to the correct uniforms as the scene graph is traversed.
    scene_state.position_loc        = position_loc_;
    scene_state.normal_loc          = normal_loc_;
    scene_state.texture_loc         = -1;  // no texture in planet shader
    scene_state.pvm_matrix_loc      = pvm_matrix_loc_;
    scene_state.model_matrix_loc    = model_matrix_loc_;
    scene_state.normal_matrix_loc   = normal_matrix_loc_;
    scene_state.camera_position_loc = camera_position_loc_;

    scene_state.material_ambient_loc   = material_ambient_loc_;
    scene_state.material_diffuse_loc   = material_diffuse_loc_;
    scene_state.material_specular_loc  = material_specular_loc_;
    scene_state.material_emission_loc  = material_emission_loc_;
    scene_state.material_shininess_loc = material_shininess_loc_;

    scene_state.use_texture_loc  = -1;
    scene_state.texture_unit_loc = -1;

    scene_state.lightcount_loc = light_count_loc_;
    for (int32_t i = 0; i < light_count_; i++)
        scene_state.lights[i] = lights_[i];

    SceneNode::draw(scene_state);
}

void PlanetShaderNode::set_global_ambient(const Color4 &global_ambient)
{
    shader_program_.use();
    glUniform4fv(global_ambient_loc_, 1, &global_ambient.r);
}

void PlanetShaderNode::set_noise_params(float amplitude, float frequency)
{
    shader_program_.use();
    glUniform1f(noise_amplitude_loc_, amplitude);
    glUniform1f(noise_frequency_loc_, frequency);
}

void PlanetShaderNode::set_elevation_color(bool enabled)
{
    shader_program_.use();
    glUniform1i(use_elevation_color_loc_, enabled ? 1 : 0);
}

int32_t PlanetShaderNode::get_position_loc() const { return position_loc_; }
int32_t PlanetShaderNode::get_normal_loc()   const { return normal_loc_;   }

} // namespace cg
