//============================================================================
//  atmosphere.vert
//  JHU_605.767 Final Project
//
//  Vertex shader for the atmosphere icosphere shell. Transforms vertices
//  to world space and passes the world-space position to the fragment
//  shader so it can reconstruct the view ray direction per-fragment.
//============================================================================

#version 410 core

layout (location = 0) in vec3 vtx_position;
layout (location = 1) in vec3 vtx_normal;

layout (location = 0) smooth out vec3 world_vertex;
layout (location = 1) smooth out vec3 world_normal;

uniform mat4 pvm_matrix;
uniform mat4 model_matrix;
uniform mat4 normal_matrix;

void main()
{
    world_vertex = vec3(model_matrix * vec4(vtx_position, 1.0));
    world_normal = normalize(vec3(normal_matrix * vec4(vtx_normal, 0.0)));
    gl_Position  = pvm_matrix * vec4(vtx_position, 1.0);
}

