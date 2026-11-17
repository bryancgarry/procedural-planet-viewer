//============================================================================
//  cloud.vert
//  JHU_605.767 Final Project
//
//  Vertex shader for the animated cloud layer. The cloud mesh is a smooth
//  icosphere at world radius 33 (slightly above sea level). No displacement
//  is applied — cloud shape comes entirely from the fragment shader's
//  noise-based transparency.
//
//  The unit-sphere position is passed through as sphere_position to give
//  the fragment shader a scale-independent coordinate for sampling noise.
//  This ensures cloud patterns don't stretch or compress with the transform.
//============================================================================

#version 410 core

layout (location = 0) in vec3 vtx_position;
layout (location = 1) in vec3 vtx_normal;

layout (location = 0) smooth out vec3 world_normal;
layout (location = 1) smooth out vec3 world_vertex;
layout (location = 2) smooth out vec3 sphere_position;

uniform mat4 pvm_matrix;
uniform mat4 model_matrix;
uniform mat4 normal_matrix;

void main()
{
    sphere_position = vtx_position;
    world_normal    = normalize(vec3(normal_matrix * vec4(vtx_normal,   0.0)));
    world_vertex    = vec3(model_matrix * vec4(vtx_position, 1.0));
    gl_Position     = pvm_matrix * vec4(vtx_position, 1.0);
}
