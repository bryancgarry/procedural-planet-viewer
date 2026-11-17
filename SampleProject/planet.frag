//============================================================================
//  planet.frag
//  JHU_605.767 Final Project
//
//  Fragment shader for the procedural planet terrain. Computes Blinn-Phong
//  lighting using the displaced surface normal from planet.vert, then
//  applies elevation-based color bands to produce ocean, shore, lowland,
//  highland, and snow zones based on the fBm noise value passed from the
//  vertex shader.
//
//  Elevation coloring can be toggled via the use_elevation_color uniform
//  so the same shader works for both the planet and the moon (which uses
//  a grey material color instead).
//============================================================================

#version 410 core

layout (location = 0) smooth in vec3  normal;
layout (location = 1) smooth in vec3  vertex;
layout (location = 2) smooth in float elevation;

layout (location = 0) out vec4 frag_color;

uniform vec4  material_ambient;
uniform vec4  material_diffuse;
uniform vec4  material_specular;
uniform vec4  material_emission;
uniform float material_shininess;

uniform vec4 global_light_ambient;
uniform vec3 camera_position;
uniform int  num_lights;

const int MAX_LIGHTS = 8;
struct LightSource {
    int   enabled;
    int   spotlight;
    vec4  position;
    vec4  ambient;
    vec4  diffuse;
    vec4  specular;
    float constant_attenuation;
    float linear_attenuation;
    float quadratic_attenuation;
    float spot_cutoff;
    float spot_exponent;
    vec3  spot_direction;
};
uniform LightSource lights[MAX_LIGHTS];

// When true, elevation drives color (planet mode).
// When false, material_diffuse is used directly (moon mode).
uniform bool use_elevation_color;

// ---------------------------------------------------------------------------
// Lighting functions — standard Blinn-Phong model
// ---------------------------------------------------------------------------

float calculate_attenuation(in int i, in float distance)
{
    return 1.0 / (lights[i].constant_attenuation +
                  lights[i].linear_attenuation    * distance +
                  lights[i].quadratic_attenuation * distance * distance);
}

void directional_light(in int i, in vec3 N, in vec3 vtx, in vec3 V,
                       inout vec4 ambient, inout vec4 diffuse, inout vec4 specular)
{
    ambient      += lights[i].ambient;
    vec3  L       = lights[i].position.xyz;
    float N_dot_L = dot(N, L);
    if (N_dot_L > 0.0) {
        diffuse      += lights[i].diffuse * N_dot_L;
        vec3  H       = normalize(L + V);
        float N_dot_H = dot(N, H);
        if (N_dot_H > 0.0)
            specular += lights[i].specular * pow(N_dot_H, material_shininess);
    }
}

void point_light(in int i, in vec3 N, in vec3 vtx, in vec3 V,
                 inout vec4 ambient, inout vec4 diffuse, inout vec4 specular)
{
    vec3  tmp         = lights[i].position.xyz - vtx;
    float dist        = length(tmp);
    vec3  L           = tmp * (1.0 / dist);
    float attenuation = calculate_attenuation(i, dist);

    ambient      += lights[i].ambient * attenuation;
    float N_dot_L = dot(N, L);
    if (N_dot_L > 0.0) {
        diffuse      += lights[i].diffuse * attenuation * N_dot_L;
        vec3  H       = normalize(L + V);
        float N_dot_H = dot(N, H);
        if (N_dot_H > 0.0)
            specular += lights[i].specular * attenuation * pow(N_dot_H, material_shininess);
    }
}

void spot_light(in int i, in vec3 N, in vec3 vtx, in vec3 V,
                inout vec4 ambient, inout vec4 diffuse, inout vec4 specular)
{
    vec3  tmp         = lights[i].position.xyz - vtx;
    float dist        = length(tmp);
    vec3  L           = tmp * (1.0 / dist);
    float attenuation = calculate_attenuation(i, dist);

    float N_dot_L = dot(N, L);
    if (N_dot_L > 0.0) {
        float spot_effect = dot(lights[i].spot_direction, -L);
        if (spot_effect > lights[i].spot_cutoff) {
            attenuation *= pow(spot_effect, lights[i].spot_exponent);
            diffuse     += lights[i].diffuse * attenuation * N_dot_L;
            vec3  H       = normalize(L + V);
            float N_dot_H = dot(N, H);
            if (N_dot_H > 0.0)
                specular += lights[i].specular * attenuation * pow(N_dot_H, material_shininess);
        } else {
            attenuation = 0.0;
        }
    }
    ambient += lights[i].ambient * attenuation;
}

// ---------------------------------------------------------------------------

void main()
{
    vec3 n = normalize(normal);
    vec3 V = normalize(camera_position - vertex);

    vec4 ambient  = vec4(0.0);
    vec4 diffuse  = vec4(0.0);
    vec4 specular = vec4(0.0);

    for (int i = 0; i < num_lights; i++) {
        if (lights[i].enabled != 1) continue;
        if      (lights[i].position.w == 0.0) directional_light(i, n, vertex, V, ambient, diffuse, specular);
        else if (lights[i].spotlight   == 1)  spot_light       (i, n, vertex, V, ambient, diffuse, specular);
        else                                   point_light      (i, n, vertex, V, ambient, diffuse, specular);
    }

    // Elevation-based color bands — thresholds tuned to the fBm value range.
    // Negative elevation = below sea level (ocean).
    // Positive elevation rises through shore, lowland, highland to snow.
    vec4 base_diffuse;
    if (use_elevation_color) {
        if      (elevation < -0.05) base_diffuse = vec4(0.05, 0.15, 0.5,  1.0);  // deep ocean
        else if (elevation <  0.0)  base_diffuse = vec4(0.1,  0.3,  0.7,  1.0);  // shallow ocean
        else if (elevation <  0.02) base_diffuse = vec4(0.8,  0.75, 0.5,  1.0);  // shore / sand
        else if (elevation <  0.08) base_diffuse = vec4(0.2,  0.5,  0.15, 1.0);  // lowland / grass
        else if (elevation <  0.13) base_diffuse = vec4(0.35, 0.3,  0.2,  1.0);  // highland / rock
        else                        base_diffuse = vec4(0.95, 0.95, 1.0,  1.0);  // snow
    } else {
        base_diffuse = material_diffuse;
    }

    vec4 color = material_emission
               + global_light_ambient * material_ambient
               + ambient  * material_ambient
               + diffuse  * base_diffuse
               + specular * material_specular;

    frag_color = clamp(color, 0.0, 1.0);
}
