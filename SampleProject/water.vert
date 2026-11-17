//============================================================================
//  water.vert
//  JHU_605.767 Final Project
//
//  Vertex shader for the ocean water surface. The water mesh is a smooth
//  icosphere at sea level (radius = 1.0 on the unit sphere, world radius 30).
//  No noise displacement is applied — the mesh stays perfectly smooth so
//  the terrain depth buffer naturally occludes land above sea level.
//
//  The unit-sphere position is passed through as sphere_position for use
//  in the fragment shader's normal perturbation noise, which gives a
//  scale-independent coordinate space for the sparkle pattern.
//============================================================================

#version 410 core

layout (location = 0) in vec3 vtx_position;
layout (location = 1) in vec3 vtx_normal;

layout (location = 0) smooth out vec3 world_normal;
layout (location = 1) smooth out vec3 world_vertex;
layout (location = 2) smooth out vec3 sphere_position;  // unit sphere coord for noise

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
