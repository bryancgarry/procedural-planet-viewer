//============================================================================
//  cloud.frag
//  JHU_605.767 Final Project
//
//  Fragment shader for the animated cloud layer.
//
//  Cloud coverage is generated using fractal Brownian motion (fBm) noise
//  sampled at the sphere surface position, offset over time to simulate
//  wind-driven cloud drift. Two noise samples at different scales are
//  combined — large scale gives broad cloud masses, small scale adds
//  wispy detail. Where the combined noise exceeds a threshold the fragment
//  is rendered as a lit white cloud; below the threshold it is discarded.
//
//  Lighting: diffuse + ambient only — no specular since real clouds are
//  diffuse scatterers. The day/night terminator naturally darkens clouds
//  on the night side via the directional sun light.
//
//  fBm functions are prefixed cloud_ to avoid name collisions with the
//  gradient noise in planet.vert.
//============================================================================

#version 410 core

layout (location = 0) smooth in vec3 world_normal;
layout (location = 1) smooth in vec3 world_vertex;
layout (location = 2) smooth in vec3 sphere_position;

out vec4 frag_color;

uniform vec3  camera_position;
uniform vec3  sun_direction;
uniform float time;

uniform float cloud_threshold;  // noise value above which clouds appear
uniform float cloud_softness;   // smoothstep transition width for feathered edges
uniform float cloud_opacity;    // maximum cloud alpha
uniform vec3  drift_direction;  // normalized wind direction in sphere space
uniform float drift_speed;      // how fast the cloud pattern drifts

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
// fBm noise — same algorithm as planet.vert, prefixed cloud_ to avoid
// name collisions when both shaders are active in the same program.
// ---------------------------------------------------------------------------

vec3 cloud_hash3(vec3 p)
{
    p = vec3(dot(p, vec3(127.1, 311.7,  74.7)),
             dot(p, vec3(269.5, 183.3, 246.1)),
             dot(p, vec3(113.5, 271.9, 124.6)));
    return fract(sin(p) * 43758.5453123) * 2.0 - 1.0;
}

float cloud_gradient_noise(vec3 p)
{
    vec3 i = floor(p);
    vec3 f = fract(p);
    vec3 u = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);

    float n000 = dot(cloud_hash3(i + vec3(0,0,0)), f - vec3(0,0,0));
    float n100 = dot(cloud_hash3(i + vec3(1,0,0)), f - vec3(1,0,0));
    float n010 = dot(cloud_hash3(i + vec3(0,1,0)), f - vec3(0,1,0));
    float n110 = dot(cloud_hash3(i + vec3(1,1,0)), f - vec3(1,1,0));
    float n001 = dot(cloud_hash3(i + vec3(0,0,1)), f - vec3(0,0,1));
    float n101 = dot(cloud_hash3(i + vec3(1,0,1)), f - vec3(1,0,1));
    float n011 = dot(cloud_hash3(i + vec3(0,1,1)), f - vec3(0,1,1));
    float n111 = dot(cloud_hash3(i + vec3(1,1,1)), f - vec3(1,1,1));

    return mix(mix(mix(n000, n100, u.x), mix(n010, n110, u.x), u.y),
               mix(mix(n001, n101, u.x), mix(n011, n111, u.x), u.y),
               u.z);
}

float cloud_fbm(vec3 p)
{
    float value     = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;
    for (int i = 0; i < 5; i++) {
        value     += amplitude * cloud_gradient_noise(p * frequency);
        amplitude *= 0.5;
        frequency *= 2.0;
    }
    return value;
}

// ---------------------------------------------------------------------------

void main()
{
    vec3 n = normalize(world_normal);

    // Animate cloud coverage by offsetting the noise sample position over time.
    // drift_direction * time * drift_speed slides the noise pattern across
    // the sphere, simulating wind-driven movement.
    vec3 sample_pos = sphere_position + drift_direction * time * drift_speed;

    // Two fBm samples at different scales — coarse gives cloud mass shape,
    // fine scale (weighted lower) adds wispy detail within each mass.
    float noise = cloud_fbm(sample_pos * 2.5)
                + cloud_fbm(sample_pos * 6.0) * 0.4;

    // Threshold with smoothstep for soft feathered cloud edges
    float coverage = smoothstep(cloud_threshold,
                                cloud_threshold + cloud_softness,
                                noise);

    // Early discard for fully transparent fragments
    if (coverage < 0.01) discard;

    // Diffuse + ambient lighting only — clouds are not specular
    vec4 ambient = vec4(0.0);
    vec4 diffuse = vec4(0.0);

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

        float N_dot_L = max(dot(n, L), 0.0);
        diffuse += lights[i].diffuse * attenuation * N_dot_L;
    }

    // Cloud color is pure white — all color variation comes from lighting
    vec4 cloud_color = vec4(1.0);
    vec4 color = global_light_ambient * cloud_color * 0.5
               + ambient * cloud_color * 0.2
               + diffuse * cloud_color;

    float alpha = coverage * cloud_opacity;
    frag_color  = clamp(vec4(color.rgb, alpha), 0.0, 1.0);
}
