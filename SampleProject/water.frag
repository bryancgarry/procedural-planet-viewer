//============================================================================
//  water.frag
//  JHU_605.767 Final Project
//
//  Ocean surface shader. Implements three visual effects:
//
//  1. Hash-based normal perturbation — breaks the single smooth-sphere
//     specular highlight into many small sparkles resembling sunlight
//     on real water. Uses a value noise function sampled at the unit-sphere
//     position (scale-independent, no UV coordinates needed).
//
//  2. Fresnel reflectance — grazing angles (looking across the water)
//     reflect the sky color while normal incidence shows the deep ocean
//     color. Implemented via Schlick's approximation.
//
//  3. Depth-based color — world radius slightly above 30 (sea level) maps
//     to shallow/coastal color; exactly at 30 maps to deep ocean color.
//     The terrain depth buffer handles land occlusion automatically.
//============================================================================

#version 410 core

layout (location = 0) smooth in vec3 world_normal;
layout (location = 1) smooth in vec3 world_vertex;
layout (location = 2) smooth in vec3 sphere_position;

out vec4 frag_color;

uniform vec3  camera_position;
uniform vec3  sun_direction;

uniform vec4  water_shallow_color;  // color near shorelines
uniform vec4  water_deep_color;     // color in deep ocean
uniform float water_shininess;      // specular exponent — high = tight glints
uniform vec3  sky_color;            // reflected at grazing angles (Fresnel)
uniform float fresnel_strength;     // strength of Fresnel edge effect

uniform vec4 global_light_ambient;
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

// ---------------------------------------------------------------------------
// Hash-based value noise for specular normal perturbation.
// Functions are prefixed water_ to avoid name collisions with
// the gradient noise in planet.vert and cloud.frag.
// ---------------------------------------------------------------------------

vec3 water_hash3(vec3 p)
{
    p = vec3(dot(p, vec3(127.1, 311.7,  74.7)),
             dot(p, vec3(269.5, 183.3, 246.1)),
             dot(p, vec3(113.5, 271.9, 124.6)));
    return fract(sin(p) * 43758.5453123) * 2.0 - 1.0;
}

float water_noise3(vec3 p)
{
    vec3 i = floor(p);
    vec3 f = fract(p);
    vec3 u = f * f * (3.0 - 2.0 * f);  // smoothstep

    return mix(
        mix(
            mix(dot(water_hash3(i + vec3(0,0,0)), f - vec3(0,0,0)),
                dot(water_hash3(i + vec3(1,0,0)), f - vec3(1,0,0)), u.x),
            mix(dot(water_hash3(i + vec3(0,1,0)), f - vec3(0,1,0)),
                dot(water_hash3(i + vec3(1,1,0)), f - vec3(1,1,0)), u.x), u.y),
        mix(
            mix(dot(water_hash3(i + vec3(0,0,1)), f - vec3(0,0,1)),
                dot(water_hash3(i + vec3(1,0,1)), f - vec3(1,0,1)), u.x),
            mix(dot(water_hash3(i + vec3(0,1,1)), f - vec3(0,1,1)),
                dot(water_hash3(i + vec3(1,1,1)), f - vec3(1,1,1)), u.x), u.y),
        u.z);
}

// Tilt the surface normal slightly using noise to break up the specular
// highlight into sparkles. The Fresnel calculation uses the unperturbed
// normal so the rim effect stays smooth.
// @param  scale     Noise spatial frequency — higher = smaller sparkles
// @param  strength  How much the normal tilts — keep 0.05-0.15 for realism
vec3 perturb_normal(vec3 base_normal, float scale, float strength)
{
    vec3 up        = abs(base_normal.z) < 0.99 ? vec3(0,0,1) : vec3(1,0,0);
    vec3 tangent   = normalize(cross(up, base_normal));
    vec3 bitangent = normalize(cross(base_normal, tangent));

    vec3  p  = sphere_position * scale;
    float nx = water_noise3(p);
    float ny = water_noise3(p + vec3(31.7, 17.3, 5.1));

    return normalize(base_normal + tangent   * nx * strength
                                 + bitangent * ny * strength);
}

// ---------------------------------------------------------------------------

void main()
{
    // Perturbed normal for specular sparkles
    vec3 n = perturb_normal(normalize(world_normal), 30.0, 0.12);
    vec3 V = normalize(camera_position - world_vertex);

    // --- Depth-based color ---
    // Planet radius is 30. Shallow coast areas are fractionally above 30
    // due to terrain mesh slightly intersecting. depth_factor goes from
    // 0 (deep ocean, pure water_deep_color) to 1 (coast, water_shallow_color).
    float world_radius = length(world_vertex);
    float depth_factor = clamp((world_radius - 29.8) / 0.5, 0.0, 1.0);
    vec4  depth_color  = mix(water_deep_color, water_shallow_color, depth_factor);

    // --- Fresnel reflectance ---
    // Use unperturbed normal for smooth rim — the normal perturbation is
    // only for the specular highlight, not the overall edge reflection.
    float cos_angle  = max(dot(normalize(world_normal), V), 0.0);
    float fresnel    = pow(1.0 - cos_angle, 4.0) * fresnel_strength;
    vec3  base_color = mix(depth_color.rgb, sky_color, fresnel);

    // --- Blinn-Phong lighting ---
    vec4 ambient  = vec4(0.0);
    vec4 diffuse  = vec4(0.0);
    vec4 specular = vec4(0.0);

    for (int i = 0; i < num_lights; i++) {
        if (lights[i].enabled != 1) continue;

        vec3  L;
        float attenuation = 1.0;

        if (lights[i].position.w == 0.0) {
            L = normalize(lights[i].position.xyz);
        } else {
            vec3  lv   = lights[i].position.xyz - world_vertex;
            float dist = length(lv);
            L           = normalize(lv);
            attenuation = 1.0 / (lights[i].constant_attenuation +
                                  lights[i].linear_attenuation    * dist +
                                  lights[i].quadratic_attenuation * dist * dist);
        }

        ambient += lights[i].ambient * attenuation;

        float N_dot_L = dot(n, L);
        if (N_dot_L > 0.0) {
            diffuse += lights[i].diffuse * attenuation * N_dot_L;
            vec3  H       = normalize(L + V);
            float N_dot_H = max(dot(n, H), 0.0);
            specular += lights[i].specular * attenuation
                        * pow(N_dot_H, water_shininess);
        }
    }

    vec4 color = global_light_ambient * vec4(base_color, 1.0) * 0.3
               + ambient  * vec4(base_color, 1.0) * 0.2
               + diffuse  * vec4(base_color, 1.0)
               + specular * vec4(1.0, 1.0, 1.0, 1.0);

    // Slightly transparent near shorelines, fully opaque in deep ocean
    float alpha = mix(0.85, 1.0, 1.0 - depth_factor);
    frag_color  = clamp(vec4(color.rgb, alpha), 0.0, 1.0);
}
