//============================================================================
//  water_shader_node.cpp
//  JHU_605.767 Final Project
//============================================================================

#include "SampleProject/water_shader_node.hpp"
#include <cstdio>
#include <iostream>

namespace cg
{

bool WaterShaderNode::get_locations()
{
    GLuint prog = shader_program_.get_program();

    position_loc_ = glGetAttribLocation(prog, "vtx_position");
    if (position_loc_ < 0) {
        std::cout << "WaterShaderNode: Error getting vtx_position\n";
        return false;
    }
    normal_loc_ = glGetAttribLocation(prog, "vtx_normal");
    if (normal_loc_ < 0) {
        std::cout << "WaterShaderNode: Error getting vtx_normal\n";
        return false;
    }

    pvm_matrix_loc_      = glGetUniformLocation(prog, "pvm_matrix");
    model_matrix_loc_    = glGetUniformLocation(prog, "model_matrix");
    normal_matrix_loc_   = glGetUniformLocation(prog, "normal_matrix");
    camera_position_loc_ = glGetUniformLocation(prog, "camera_position");

    global_ambient_loc_ = glGetUniformLocation(prog, "global_light_ambient");
    light_count_        = 3;
    light_count_loc_    = glGetUniformLocation(prog, "num_lights");

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

    sun_direction_loc_       = glGetUniformLocation(prog, "sun_direction");
    water_shallow_color_loc_ = glGetUniformLocation(prog, "water_shallow_color");
    water_deep_color_loc_    = glGetUniformLocation(prog, "water_deep_color");
    water_shininess_loc_     = glGetUniformLocation(prog, "water_shininess");
    sky_color_loc_           = glGetUniformLocation(prog, "sky_color");
    fresnel_strength_loc_    = glGetUniformLocation(prog, "fresnel_strength");

    return true;
}

void WaterShaderNode::draw(SceneState &scene_state)
{
    shader_program_.use();

    scene_state.position_loc         = position_loc_;
    scene_state.normal_loc           = normal_loc_;
    scene_state.texture_loc          = -1;
    scene_state.pvm_matrix_loc       = pvm_matrix_loc_;
    scene_state.model_matrix_loc     = model_matrix_loc_;
    scene_state.normal_matrix_loc    = normal_matrix_loc_;
    scene_state.camera_position_loc  = camera_position_loc_;
    scene_state.material_ambient_loc   = -1;
    scene_state.material_diffuse_loc   = -1;
    scene_state.material_specular_loc  = -1;
    scene_state.material_emission_loc  = -1;
    scene_state.material_shininess_loc = -1;
    scene_state.use_texture_loc        = -1;
    scene_state.texture_unit_loc       = -1;
    scene_state.lightcount_loc         = light_count_loc_;
    for (int i = 0; i < light_count_; i++)
        scene_state.lights[i] = lights_[i];

    // Camera already ran before this shader activated — push its position
    // directly since it won't write to this program's uniforms automatically.
    glUniform3fv(camera_position_loc_, 1, &scene_state.camera_position.x);

    // Alpha blending for shoreline transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    SceneNode::draw(scene_state);

    glDisable(GL_BLEND);
}

void WaterShaderNode::set_global_ambient(const Color4 &global_ambient)
{
    shader_program_.use();
    glUniform4fv(global_ambient_loc_, 1, &global_ambient.r);
}

void WaterShaderNode::set_water_params(const Vector3 &sun_dir,
                                        const Color4  &shallow_color,
                                        const Color4  &deep_color,
                                        float          shininess,
                                        const Vector3 &sky_color,
                                        float          fresnel_strength)
{
    shader_program_.use();
    glUniform3fv(sun_direction_loc_,       1, &sun_dir.x);
    glUniform4fv(water_shallow_color_loc_, 1, &shallow_color.r);
    glUniform4fv(water_deep_color_loc_,    1, &deep_color.r);
    glUniform1f(water_shininess_loc_,      shininess);
    glUniform3fv(sky_color_loc_,           1, &sky_color.x);
    glUniform1f(fresnel_strength_loc_,     fresnel_strength);
}

int32_t WaterShaderNode::get_position_loc() const { return position_loc_; }
int32_t WaterShaderNode::get_normal_loc()   const { return normal_loc_;   }

} // namespace cg
