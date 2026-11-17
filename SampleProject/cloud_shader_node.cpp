//============================================================================
//  cloud_shader_node.cpp
//  JHU_605.767 Final Project
//============================================================================

#include "SampleProject/cloud_shader_node.hpp"
#include <cstdio>
#include <iostream>

namespace cg
{

bool CloudShaderNode::get_locations()
{
    GLuint prog = shader_program_.get_program();

    position_loc_ = glGetAttribLocation(prog, "vtx_position");
    if (position_loc_ < 0) {
        std::cout << "CloudShaderNode: Error getting vtx_position\n";
        return false;
    }
    normal_loc_ = glGetAttribLocation(prog, "vtx_normal");
    if (normal_loc_ < 0) {
        std::cout << "CloudShaderNode: Error getting vtx_normal\n";
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

    sun_direction_loc_   = glGetUniformLocation(prog, "sun_direction");
    time_loc_            = glGetUniformLocation(prog, "time");
    cloud_threshold_loc_ = glGetUniformLocation(prog, "cloud_threshold");
    cloud_softness_loc_  = glGetUniformLocation(prog, "cloud_softness");
    cloud_opacity_loc_   = glGetUniformLocation(prog, "cloud_opacity");
    drift_direction_loc_ = glGetUniformLocation(prog, "drift_direction");
    drift_speed_loc_     = glGetUniformLocation(prog, "drift_speed");

    return true;
}

void CloudShaderNode::draw(SceneState &scene_state)
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

    // Alpha blending for transparent cloud fragments.
    // Depth mask off so clouds don't occlude the atmosphere shell behind them.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    SceneNode::draw(scene_state);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void CloudShaderNode::set_global_ambient(const Color4 &global_ambient)
{
    shader_program_.use();
    glUniform4fv(global_ambient_loc_, 1, &global_ambient.r);
}

void CloudShaderNode::set_cloud_params(const Vector3 &sun_dir,
                                        float          threshold,
                                        float          softness,
                                        float          opacity,
                                        const Vector3 &drift_dir,
                                        float          drift_speed)
{
    shader_program_.use();
    glUniform3fv(sun_direction_loc_,   1, &sun_dir.x);
    glUniform1f(cloud_threshold_loc_,  threshold);
    glUniform1f(cloud_softness_loc_,   softness);
    glUniform1f(cloud_opacity_loc_,    opacity);
    glUniform3fv(drift_direction_loc_, 1, &drift_dir.x);
    glUniform1f(drift_speed_loc_,      drift_speed);
}

void CloudShaderNode::set_time(float time)
{
    shader_program_.use();
    glUniform1f(time_loc_, time);
}

int32_t CloudShaderNode::get_position_loc() const { return position_loc_; }
int32_t CloudShaderNode::get_normal_loc()   const { return normal_loc_;   }

} // namespace cg
