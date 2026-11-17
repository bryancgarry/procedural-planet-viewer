//============================================================================
//  atmosphere.frag
//  JHU_605.767 Final Project
//
//  Ray-marched atmosphere on an icosphere shell using Rayleigh and Mie
//  scattering. For each fragment on the shell, a ray is cast from the
//  camera position through that fragment's world-space position and marched
//  through the atmosphere volume, accumulating scattered light.
//
//  The planet surface is not explicitly occluded — the depth buffer handles
//  occlusion by the planet and moon geometry since the atmosphere shell is
//  rendered with depth testing enabled and depth writes disabled. Rays that
//  pass through the planet disc accumulate less scattering than limb rays,
//  producing a natural smooth gradient rather than a hard cutoff.
//
//  When the camera is inside the atmosphere shell, alpha is boosted so the
//  sky fills in correctly rather than showing the skybox through thin paths.
//
//  Scattering model adapted from:
//  Nishita et al., "Display of The Earth Taking into Account Atmospheric
//  Scattering", SIGGRAPH 1993.
//============================================================================

#version 410 core

layout (location = 0) smooth in vec3 world_vertex;
layout (location = 1) smooth in vec3 world_normal;

out vec4 frag_color;

// Per-frame — set by AtmosphereShaderNode::draw()
uniform vec3  camera_position;

// Static — set once by AtmosphereShaderNode::set_params()
uniform float planet_radius;       // density falloff anchor (not visual surface)
uniform float atmosphere_radius;   // outer shell radius — must match geometry scale
uniform vec3  sun_direction;       // normalized FROM planet TOWARD sun
uniform float sun_intensity;       // overall brightness multiplier

// Scattering coefficients — tuned for vivid blue at this world scale.
// Blue channel of rayleigh_coeffs is much higher than red/green to
// bias scattering toward blue (simulates Earth's sky color).
// Mie is kept near zero to suppress the brown/orange haze it produces.
uniform vec3  rayleigh_coeffs;        // wavelength-dependent Rayleigh scatter
uniform float mie_coeff;              // wavelength-independent Mie scatter
uniform float rayleigh_scale_height;  // Rayleigh density falloff rate
uniform float mie_scale_height;       // Mie density falloff rate

const int VIEW_STEPS = 16;  // ray march samples along view ray
const int SUN_STEPS  = 8;   // samples toward sun per view sample

// ---------------------------------------------------------------------------
// Ray-sphere intersection. Returns false if ray misses sphere.
// t0 = entry distance, t1 = exit distance (may be negative if behind camera).
// ---------------------------------------------------------------------------
bool ray_sphere(vec3 origin, vec3 dir, float radius, out float t0, out float t1)
{
    float a    = dot(dir, dir);
    float b    = 2.0 * dot(origin, dir);
    float c    = dot(origin, origin) - radius * radius;
    float disc = b * b - 4.0 * a * c;
    if (disc < 0.0) return false;
    float sq = sqrt(disc);
    t0 = (-b - sq) / (2.0 * a);
    t1 = (-b + sq) / (2.0 * a);
    return true;
}

// Atmosphere density at a world-space point — exponential falloff with altitude
vec2 atmosphere_density(vec3 point)
{
    float altitude = length(point) - planet_radius;
    return vec2(exp(-altitude / rayleigh_scale_height),
                exp(-altitude / mie_scale_height));
}

// Optical depth from a sample point toward the sun — how much sunlight
// is absorbed before reaching this point (used to darken the night side)
vec2 optical_depth_sun(vec3 point)
{
    float t0, t1;
    ray_sphere(point, sun_direction, atmosphere_radius, t0, t1);
    float step_size = t1 / float(SUN_STEPS);
    vec2  depth     = vec2(0.0);
    for (int i = 0; i < SUN_STEPS; i++) {
        vec3 p = point + sun_direction * (float(i) + 0.5) * step_size;
        depth += atmosphere_density(p) * step_size;
    }
    return depth;
}

// Rayleigh phase function — symmetric scattering, produces blue sky
float rayleigh_phase(float cos_theta)
{
    return (3.0 / (16.0 * 3.14159265)) * (1.0 + cos_theta * cos_theta);
}

// Mie phase function (Cornette-Shanks) — forward-biased, produces sun halo
float mie_phase(float cos_theta)
{
    const float g = 0.8;
    float g2      = g * g;
    return (3.0 / (8.0 * 3.14159265))
         * ((1.0 - g2) * (1.0 + cos_theta * cos_theta))
         / ((2.0 + g2) * pow(1.0 + g2 - 2.0 * g * cos_theta, 1.5));
}

// Main ray march — integrates Rayleigh and Mie scattering along the view ray
vec3 compute_atmosphere(vec3 ray_origin, vec3 ray_dir)
{
    float atmo_t0, atmo_t1;
    if (!ray_sphere(ray_origin, ray_dir, atmosphere_radius, atmo_t0, atmo_t1))
        return vec3(0.0);

    float t_start = max(atmo_t0, 0.0);
    float t_end   = atmo_t1;
    if (t_end <= t_start) return vec3(0.0);

    float step_size          = (t_end - t_start) / float(VIEW_STEPS);
    vec2  optical_depth_view = vec2(0.0);
    vec3  rayleigh_scatter   = vec3(0.0);
    vec3  mie_scatter        = vec3(0.0);

    for (int i = 0; i < VIEW_STEPS; i++) {
        float t            = t_start + (float(i) + 0.5) * step_size;
        vec3  sample_point = ray_origin + ray_dir * t;
        vec2  density      = atmosphere_density(sample_point) * step_size;
        optical_depth_view += density;

        vec2 depth_sun   = optical_depth_sun(sample_point);
        vec2 total_depth = optical_depth_view + depth_sun;

        // Beer-Lambert transmittance — fraction of light surviving this path
        vec3 transmittance = exp(
            -(rayleigh_coeffs * total_depth.x + mie_coeff * total_depth.y * 1.1)
        );

        rayleigh_scatter += density.x * transmittance;
        mie_scatter      += density.y * transmittance;
    }

    float cos_theta = dot(ray_dir, sun_direction);
    return sun_intensity * (
        rayleigh_scatter * rayleigh_coeffs * rayleigh_phase(cos_theta)
      + mie_scatter      * mie_coeff       * mie_phase(cos_theta)
    );
}

// ---------------------------------------------------------------------------

void main()
{
    // View ray direction from camera through this shell fragment.
    // The fragment's world-space position gives the ray direction directly —
    // no screen-space reconstruction or inverse matrix needed.
    vec3 ray_dir = normalize(world_vertex - camera_position);

    vec3 color = compute_atmosphere(camera_position, ray_dir);

    // Reinhard tone mapping — brings physically-scaled values into [0,1]
    color = color / (color + vec3(1.0));

    // Alpha driven by luminance with a power curve — naturally peaks at the
    // limb (long path through atmosphere = bright) and fades over the disc.
    float lum   = dot(color, vec3(0.299, 0.587, 0.114));
    float alpha = clamp(pow(lum, 2.5) * 5.0, 0.0, 1.0);

    // When camera is inside the atmosphere, boost alpha so the sky fills in
    // rather than showing the skybox through thin near-zenith paths.
    float camera_dist = length(camera_position);
    if (camera_dist < atmosphere_radius) {
        float camera_altitude = clamp(
            (camera_dist - planet_radius) / (atmosphere_radius - planet_radius),
            0.0, 1.0);
        float inside_boost = pow(1.0 - camera_altitude, 2.0) * 3.0;
        alpha = clamp(alpha + inside_boost * lum, 0.0, 1.0);
    }

    frag_color = vec4(color, alpha);
}
