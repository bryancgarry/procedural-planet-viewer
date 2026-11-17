//============================================================================
//  planet.vert
//  JHU_605.767 Final Project
//
//  Vertex shader for the procedural planet terrain. Displaces each vertex
//  along its normal by a fractal Brownian motion (fBm) noise value, then
//  approximates the displaced normal using central differences so lighting
//  correctly follows the terrain contours.
//============================================================================

#version 410 core

layout (location = 0) in vec3 vtx_position;
layout (location = 1) in vec3 vtx_normal;

// World-space outputs for fragment shader lighting
layout (location = 0) smooth out vec3  normal;
layout (location = 1) smooth out vec3  vertex;
layout (location = 2) smooth out float elevation;

uniform mat4 pvm_matrix;
uniform mat4 model_matrix;
uniform mat4 normal_matrix;

// Terrain controls — must match values passed to PlanetShaderNode::set_noise_params()
uniform float noise_amplitude;  // vertical scale of terrain features
uniform float noise_frequency;  // horizontal scale (higher = more detail)

// ---------------------------------------------------------------------------
// Gradient noise (Perlin-style).
// Adapted from Stefan Gustavson, "Simplex Noise Demystified" (2005), public domain.
// https://itn-web.it.liu.se/~stegu76/TNM084-2011/simplexnoise-demystified.pdf
// ---------------------------------------------------------------------------

// Pseudo-random gradient vector from a lattice point
vec3 hash3(vec3 p)
{
    p = vec3(dot(p, vec3(127.1, 311.7,  74.7)),
             dot(p, vec3(269.5, 183.3, 246.1)),
             dot(p, vec3(113.5, 271.9, 124.6)));
    return fract(sin(p) * 43758.5453123) * 2.0 - 1.0;
}

// 3D gradient noise — returns a value in roughly [-1, 1]
float gradient_noise(vec3 p)
{
    vec3 i = floor(p);
    vec3 f = fract(p);

    // Perlin's quintic smoothstep for C2 continuity
    vec3 u = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);

    float n000 = dot(hash3(i + vec3(0,0,0)), f - vec3(0,0,0));
    float n100 = dot(hash3(i + vec3(1,0,0)), f - vec3(1,0,0));
    float n010 = dot(hash3(i + vec3(0,1,0)), f - vec3(0,1,0));
    float n110 = dot(hash3(i + vec3(1,1,0)), f - vec3(1,1,0));
    float n001 = dot(hash3(i + vec3(0,0,1)), f - vec3(0,0,1));
    float n101 = dot(hash3(i + vec3(1,0,1)), f - vec3(1,0,1));
    float n011 = dot(hash3(i + vec3(0,1,1)), f - vec3(0,1,1));
    float n111 = dot(hash3(i + vec3(1,1,1)), f - vec3(1,1,1));

    return mix(mix(mix(n000, n100, u.x), mix(n010, n110, u.x), u.y),
               mix(mix(n001, n101, u.x), mix(n011, n111, u.x), u.y),
               u.z);
}

// Fractal Brownian Motion — 5 octaves of layered noise.
// Each octave doubles the frequency and halves the amplitude,
// producing coarse landmasses with fine surface detail.
float fbm(vec3 p)
{
    float value     = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;

    for (int i = 0; i < 5; i++) {
        value     += amplitude * gradient_noise(p * frequency);
        amplitude *= 0.5;
        frequency *= 2.0;
    }
    return value;
}

// ---------------------------------------------------------------------------

void main()
{
    // Sample noise at this vertex's unit-sphere position
    float noise = fbm(vtx_position * noise_frequency);

    // Displace vertex outward along its normal by the noise amount.
    // On a unit sphere vtx_normal == vtx_position (normalized), so this
    // pushes the vertex radially inward or outward to form terrain.
    vec3 displaced = vtx_position + vtx_normal * noise * noise_amplitude;

    // Approximate the displaced surface normal using central differences.
    // We sample noise at two nearby points in the tangent plane and
    // compute the cross product of the resulting displaced tangent vectors.
    const float eps = 0.001;

    vec3 tangent   = normalize(abs(vtx_normal.x) < 0.9
                        ? vec3(0.0,  vtx_normal.z, -vtx_normal.y)
                        : vec3(-vtx_normal.z, 0.0,  vtx_normal.x));
    vec3 bitangent = cross(vtx_normal, tangent);

    float n_t = fbm((vtx_position + tangent   * eps) * noise_frequency);
    float n_b = fbm((vtx_position + bitangent * eps) * noise_frequency);

    vec3 p_t = (vtx_position + tangent   * eps) + vtx_normal * n_t * noise_amplitude;
    vec3 p_b = (vtx_position + bitangent * eps) + vtx_normal * n_b * noise_amplitude;

    vec3 displaced_normal = normalize(cross(p_t - displaced, p_b - displaced));

    // Output elevation (raw noise value) for fragment shader coloring
    elevation = noise;

    // Transform to world space
    normal    = normalize(vec3(normal_matrix * vec4(displaced_normal, 0.0)));
    vertex    = vec3(model_matrix * vec4(displaced, 1.0));
    gl_Position = pvm_matrix * vec4(displaced, 1.0);
}
