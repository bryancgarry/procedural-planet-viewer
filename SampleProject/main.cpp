//============================================================================
//  Johns Hopkins University Engineering Programs for Professionals
//  605.767 Applied Computer Graphics
//  Instructor: Brian Russin
//
//  Author:  Bryan Garry
//  File:    SampleProject/main.cpp
//  Purpose: Final project - procedural planet viewer. Renders a procedurally
//           generated planet with vertex shader terrain displacement, elevation
//           based coloring, ray-marched atmosphere, ocean and cloud
//           layers, an orbiting moon, and a flyable spacecraft with 6DOF
//           controls and bounding sphere collision detection.
//============================================================================

#include "common/event.hpp"
#include "common/logging.hpp"
#include "filesystem_support/file_locator.hpp"
#include "geometry/geometry.hpp"
#include "scene/graphics.hpp"
#include "scene/scene.hpp"

#include "SampleProject/lighting_shader_node.hpp"
#include "SampleProject/shader_src.hpp"
#include "SampleProject/planet_shader_node.hpp"
#include "SampleProject/atmosphere_shader_node.hpp"
#include "SampleProject/water_shader_node.hpp"
#include "SampleProject/cloud_shader_node.hpp"
#include "SampleProject/spacecraft.hpp"
#include "SampleProject/skybox.hpp"
#include "SampleProject/skybox_shader_node.hpp"

#include "scene/icosphere.hpp"
#include "scene/water_node.hpp"

#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

//============================================================================
// SDL / OpenGL setup
//============================================================================

SDL_Window       *g_sdl_window = nullptr;
SDL_GLContext     g_gl_context;
constexpr int32_t DRAWS_PER_SECOND     = 72;
constexpr int32_t DRAW_INTERVAL_MILLIS =
    static_cast<int32_t>(1000.0 / static_cast<double>(DRAWS_PER_SECOND));

//============================================================================
// Scene graph
//============================================================================

std::shared_ptr<cg::SceneNode>  g_scene_root;
std::shared_ptr<cg::CameraNode> g_camera;
std::shared_ptr<cg::LightNode>  Spotlight;
cg::SceneState                  g_scene_state;

//============================================================================
// Sun direction — shared by planet lighting, water, clouds, and atmosphere.
// Normalized vector pointing FROM the planet TOWARD the sun.
// Change this single value to reposition the sun across all systems.
//============================================================================

const cg::Vector3 g_sun_direction = cg::Vector3(1.0f, 0.0f, 0.0f);

//============================================================================
// Shader globals — kept global so the main loop can push per-frame updates
//============================================================================

std::shared_ptr<cg::AtmosphereShaderNode> g_atmo_shader;
std::shared_ptr<cg::WaterShaderNode>      g_water_shader;
std::shared_ptr<cg::CloudShaderNode>      g_cloud_shader;
std::shared_ptr<cg::SkyboxShaderNode>     g_skybox_shader;
std::shared_ptr<cg::Skybox>               g_skybox;

//============================================================================
// Camera / input state
//============================================================================

bool    g_animate      = false;
bool    g_forward      = true;
float   g_velocity     = 1.0f;
int32_t g_mouse_x      = 0;
int32_t g_mouse_y      = 0;
int32_t g_render_width = 800;
int32_t g_render_height= 600;

// Orbit camera
bool  orbit_mode      = false;
float orbit_distance  = 100.0f;
float orbit_azimuth   = 0.0f;
float orbit_elevation = 20.0f;
int   last_mouse_x    = 0;
int   last_mouse_y    = 0;
bool  mouse_dragging  = false;

//============================================================================
// Animation time — incremented each frame, used by clouds and water
//============================================================================

float g_time      = 0.0f;
auto  g_last_time = std::chrono::high_resolution_clock::now();

//============================================================================
// Spacecraft
//============================================================================

cg::Spacecraft g_spacecraft;
bool           spacecraft_mode = false;
bool           first_person    = false;
float          spacecraft_speed = 2.0f;  // thrust impulse per keypress
float          spacecraft_rot   = 1.5f;  // rotation degrees per keypress

// Two stacked transform nodes — translation and rotation kept separate so
// the normal matrix is always computed from a pure rotation (no translation),
// which guarantees correct lighting regardless of ship orientation.
std::shared_ptr<cg::TransformNode> g_ship_translate;
std::shared_ptr<cg::TransformNode> g_ship_rotate;

//============================================================================
// Moon
//============================================================================

std::shared_ptr<cg::TransformNode> g_moon_transform;
cg::Point3 g_moon_world_pos(0.0f, 90.0f, 0.0f);
float      g_moon_orbit_speed = 0.01f;  // full orbits per second

//============================================================================
// Celestial body collision parameters.
// collision_radius is a bounding sphere slightly above average terrain height.
// soi_radius defines the sphere of influence for gravity.
//============================================================================

const cg::CelestialBody g_planet_body = {
    cg::Point3(0.0f, 0.0f, 0.0f),
    30.0f,  // base radius
    32.0f,  // collision radius — above average terrain (noise_amplitude 0.55)
    60.0f   // sphere of influence radius
};

cg::CelestialBody g_moon_body = {
    cg::Point3(0.0f, 90.0f, 0.0f),  // updated each frame in update_moon_orbit
    10.0f,  // base radius
    11.0f,  // collision radius
    20.0f   // sphere of influence radius
};

//============================================================================
// Utility
//============================================================================

void sleep(int32_t milliseconds)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

//============================================================================
// reshape — update projection when window is resized
//============================================================================

/**
 * Update viewport and projection matrix on window resize.
 * @param  width   New window width in pixels
 * @param  height  New window height in pixels
 */
void reshape(int32_t width, int32_t height)
{
    g_render_width  = width;
    g_render_height = height;
    glViewport(0, 0, width, height);
    g_camera->change_aspect_ratio(
        static_cast<float>(width) / static_cast<float>(height));
}

//============================================================================
// update_spotlight — keep spotlight at camera position each frame
//============================================================================

void update_spotlight()
{
    cg::Point3  pos = g_camera->get_position();
    cg::Vector3 dir = g_camera->get_view_plane_normal() * -1.0f;
    Spotlight->set_position(cg::HPoint3(pos.x, pos.y, pos.z, 1.0f));
    Spotlight->set_spotlight_direction(dir);
}

//============================================================================
// display — clear and redraw the scene graph
//============================================================================

/**
 * Clear the framebuffer and draw the full scene graph.
 * Blending is enabled before the draw call so transparent objects
 * (atmosphere shell, water, cloud layer, sun halo) composite correctly.
 */
void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    g_scene_state.init();

    // Push skybox matrices from camera each frame
    g_skybox_shader->set_projection_matrix(g_camera->get_projection_matrix());
    g_skybox_shader->set_view_matrix(g_camera->get_view_matrix());

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    g_scene_root->draw(g_scene_state);
    glDisable(GL_BLEND);

    SDL_GL_SwapWindow(g_sdl_window);
}

//============================================================================
// Camera update functions
//============================================================================

/**
 * Free-flight camera — moves and turns based on mouse position.
 * @param  x        Mouse x position in window
 * @param  y        Mouse y position in window
 * @param  forward  True to move forward, false to move backward
 */
void update_view(int32_t x, int32_t y, bool forward)
{
    float dx = 4.0f * ((x - (static_cast<float>(g_render_width  * 0.5f))) /
                        static_cast<float>(g_render_width));
    float dy = 4.0f * (((static_cast<float>(g_render_height * 0.5f) - y)) /
                        static_cast<float>(g_render_height));
    float dz = forward ? g_velocity : -g_velocity;
    g_camera->move_and_turn(dx * g_velocity, dy * g_velocity, dz);
}

/**
 * Orbit camera — positions camera on a sphere around the planet origin
 * at the current azimuth, elevation, and distance.
 */
void update_orbit_camera()
{
    cg::Point3 target(0.0f, 0.0f, 0.0f);

    float az  = cg::degrees_to_radians(orbit_azimuth);
    float el  = cg::degrees_to_radians(orbit_elevation);

    cg::Point3 cam_pos(
        target.x + orbit_distance * std::cos(el) * std::sin(az),
        target.y + orbit_distance * std::cos(el) * std::cos(az),
        target.z + orbit_distance * std::sin(el));

    g_camera->set_position(cam_pos);
    g_camera->set_look_at_pt(target);
    g_camera->set_view_up(cg::Vector3(0.0f, 0.0f, 1.0f));
}

/**
 * Spacecraft camera — follows the ship in third or first person.
 * The nominal up vector is set to the ship's local up so the camera
 * rolls correctly with the ship during roll maneuvers.
 */
void update_spacecraft_camera()
{
    // Must set nominal up before position/lookat so look_at() uses it
    g_camera->set_nominal_up(g_spacecraft.get_up());

    if (first_person) {
        g_camera->set_position(g_spacecraft.get_first_person_camera_pos());
        g_camera->set_look_at_pt(g_spacecraft.get_look_at_point());
    } else {
        g_camera->set_position(g_spacecraft.get_third_person_camera_pos());
        g_camera->set_look_at_pt(g_spacecraft.get_look_at_point());
    }
}

/**
 * Push the current spacecraft position and orientation into the scene graph
 * transform nodes. Translation and rotation are applied in separate nodes so
 * the normal matrix is always derived from a pure rotation matrix, keeping
 * lighting correct after pitch, yaw, and roll operations.
 */
void update_ship_transform()
{
    const cg::Point3  &pos = g_spacecraft.get_position();
    const cg::Vector3 &fwd = g_spacecraft.get_forward();
    const cg::Vector3 &up  = g_spacecraft.get_up();
    const cg::Vector3 &rt  = g_spacecraft.get_right();

    g_ship_translate->load_identity();
    g_ship_translate->translate(pos.x, pos.y, pos.z);

    // Pure rotation matrix — inverse transpose equals itself for orthonormal R,
    // so the normal matrix is always correct regardless of ship orientation.
    cg::Matrix4x4 rot;
    rot.m00() = rt.x;  rot.m01() = up.x;  rot.m02() = fwd.x;  rot.m03() = 0.0f;
    rot.m10() = rt.y;  rot.m11() = up.y;  rot.m12() = fwd.y;  rot.m13() = 0.0f;
    rot.m20() = rt.z;  rot.m21() = up.z;  rot.m22() = fwd.z;  rot.m23() = 0.0f;
    rot.m30() = 0.0f;  rot.m31() = 0.0f;  rot.m32() = 0.0f;   rot.m33() = 1.0f;

    g_ship_rotate->load_identity();
    g_ship_rotate->set_matrix(rot);
}

//============================================================================
// Moon orbit update
//============================================================================

/**
 * Animate the moon orbit each frame based on elapsed time.
 * Also keeps g_moon_body.position in sync for gravity and collision.
 */
void update_moon_orbit()
{
    float angle      = g_time * g_moon_orbit_speed * 360.0f;
    float angle_rad  = cg::degrees_to_radians(angle);
    float orbit_r    = 90.0f;  // outside planet SOI (60) + moon SOI (20) + buffer
    float tilt       = 0.2f;   // slight orbital inclination in radians

    float x = orbit_r * std::cos(angle_rad);
    float y = orbit_r * std::sin(angle_rad);
    float z = orbit_r * std::sin(tilt) * std::sin(angle_rad);

    g_moon_world_pos     = cg::Point3(x, y, z);
    g_moon_body.position = g_moon_world_pos;

    g_moon_transform->load_identity();
    g_moon_transform->translate(x, y, z);
}

//============================================================================
// Event handlers
//============================================================================

/**
 * Mouse button handler.
 * In orbit mode, left click toggles mouse drag for azimuth/elevation.
 * Otherwise, left/right click triggers free-flight camera movement.
 */
cg::EventType handle_mouse_event(const SDL_Event &event)
{
    cg::EventType result = cg::EventType::NONE;

    if (orbit_mode)
    {
        if (event.button.button == 1)
        {
            if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
            {
                mouse_dragging = true;
                last_mouse_x   = event.button.x;
                last_mouse_y   = event.button.y;
                g_animate      = true;
                result         = cg::EventType::REDRAW;
            }
            else
            {
                g_animate      = false;
                mouse_dragging = false;
            }
        }
    }
    else
    {
        switch(event.button.button)
        {
            case SDL_BUTTON_LEFT:
                g_forward = true;
                if(event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
                {
                    g_animate = true;
                    result    = cg::EventType::REDRAW;
                }
                else g_animate = false;
                break;

            case SDL_BUTTON_RIGHT:
                g_forward = false;
                if(event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
                {
                    g_animate = true;
                    result    = cg::EventType::REDRAW;
                }
                else g_animate = false;
                break;
        }
    }
    return result;
}

/**
 * Mouse motion handler.
 * In orbit mode with drag active, updates azimuth and elevation.
 * Otherwise, updates mouse position for free-flight camera.
 */
cg::EventType handle_mouse_motion_event(const SDL_Event &event)
{
    if (orbit_mode && mouse_dragging)
    {
        int dx = event.motion.x - last_mouse_x;
        int dy = event.motion.y - last_mouse_y;

        orbit_azimuth   += dx * 0.3f;
        orbit_elevation += dy * 0.3f;
        orbit_elevation  = std::clamp(orbit_elevation, -89.0f, 89.0f);

        update_orbit_camera();

        last_mouse_x = event.motion.x;
        last_mouse_y = event.motion.y;
    }
    else
    {
        g_mouse_x = event.motion.x;
        g_mouse_y = event.motion.y;
    }

    return g_animate ? cg::EventType::REDRAW : cg::EventType::NONE;
}

/**
 * Window event handler — handles resize events.
 */
cg::EventType handle_window_event(const SDL_Event &event)
{
    cg::EventType result = cg::EventType::NONE;
    switch(event.type)
    {
        case SDL_EVENT_WINDOW_RESIZED:
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            reshape(event.window.data1, event.window.data2);
            result = cg::EventType::REDRAW;
            break;
        default: break;
    }
    return result;
}

/**
 * Keyboard event handler.
 *
 * General controls:
 *   ESC       — exit
 *   R         — reset camera to initial position
 *   TAB       — toggle orbit mode
 *   SPACE     — toggle spacecraft mode
 *
 * Orbit mode (TAB):
 *   Mouse drag  — rotate orbit azimuth and elevation
 *   Scroll      — zoom in/out
 *
 * Spacecraft mode (SPACE):
 *   W/S       — pitch up/down
 *   A/D       — yaw left/right
 *   Q/E       — roll left/right
 *   I/K       — thrust forward/backward
 *   J/L       — strafe left/right
 *   U/O       — strafe up/down
 *   C         — toggle first/third person camera
 */
cg::EventType handle_key_event(const SDL_Event &event)
{
    cg::EventType result     = cg::EventType::NONE;
    bool          upper_case = (event.key.mod & SDL_KMOD_SHIFT) ||
                               (event.key.mod & SDL_KMOD_CAPS);

    switch(event.key.key)
    {
        case SDLK_ESCAPE: result = cg::EventType::EXIT; break;

        // Reset camera to initial position
        case SDLK_R:
            g_camera->set_position(cg::Point3(0.0f, -100.0f, 0.0f));
            g_camera->set_look_at_pt(cg::Point3(0.0f, 0.0f, 0.0f));
            g_camera->set_view_up(cg::Vector3(0.0f, 0.0f, 1.0f));
            result = cg::EventType::REDRAW;
            break;

        // Toggle orbit mode
        case SDLK_TAB:
            orbit_mode = !orbit_mode;
            if (orbit_mode) update_orbit_camera();
            result = cg::EventType::REDRAW;
            break;

        // Toggle spacecraft mode
        case SDLK_SPACE:
            spacecraft_mode = !spacecraft_mode;
            orbit_mode      = false;
            if (!spacecraft_mode)
                g_camera->set_nominal_up(cg::Vector3(0.0f, 0.0f, 1.0f));
            result = cg::EventType::REDRAW;
            break;

        // Toggle first/third person camera (spacecraft mode only)
        case SDLK_C:
            if (spacecraft_mode) {
                first_person = !first_person;
                result = cg::EventType::REDRAW;
            }
            break;

        // Spacecraft rotation — pitch, yaw, roll
        case SDLK_W:
            if (spacecraft_mode) { g_spacecraft.pitch( spacecraft_rot); result = cg::EventType::REDRAW; }
            break;
        case SDLK_S:
            if (spacecraft_mode) { g_spacecraft.pitch(-spacecraft_rot); result = cg::EventType::REDRAW; }
            break;
        case SDLK_A:
            if (spacecraft_mode) { g_spacecraft.yaw( spacecraft_rot); result = cg::EventType::REDRAW; }
            break;
        case SDLK_D:
            if (spacecraft_mode) { g_spacecraft.yaw(-spacecraft_rot); result = cg::EventType::REDRAW; }
            break;
        case SDLK_Q:
            if (spacecraft_mode) { g_spacecraft.roll( spacecraft_rot); result = cg::EventType::REDRAW; }
            break;
        case SDLK_E:
            if (spacecraft_mode) { g_spacecraft.roll(-spacecraft_rot); result = cg::EventType::REDRAW; }
            break;

        // Spacecraft translation — thrust and strafe
        case SDLK_I:
            if (spacecraft_mode) { g_spacecraft.thrust( spacecraft_speed); result = cg::EventType::REDRAW; }
            break;
        case SDLK_K:
            if (spacecraft_mode) { g_spacecraft.thrust(-spacecraft_speed); result = cg::EventType::REDRAW; }
            break;
        case SDLK_J:
            if (spacecraft_mode) { g_spacecraft.strafe_right(-spacecraft_speed); result = cg::EventType::REDRAW; }
            break;
        case SDLK_L:
            if (spacecraft_mode) { g_spacecraft.strafe_right( spacecraft_speed); result = cg::EventType::REDRAW; }
            break;
        case SDLK_U:
            if (spacecraft_mode) { g_spacecraft.strafe_up( spacecraft_speed); result = cg::EventType::REDRAW; }
            break;
        case SDLK_O:
            if (spacecraft_mode) { g_spacecraft.strafe_up(-spacecraft_speed); result = cg::EventType::REDRAW; }
            break;

        default: break;
    }
    return result;
}

/**
 * Poll and dispatch all pending SDL events.
 */
cg::EventType handle_events()
{
    cg::EventType result = cg::EventType::NONE;
    SDL_Event     e;
    while(SDL_PollEvent(&e))
    {
        switch(e.type)
        {
            case SDL_EVENT_QUIT:
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                result |= cg::EventType::EXIT; break;

            case SDL_EVENT_WINDOW_RESIZED:
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                result |= handle_window_event(e); break;

            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_UP:
                result |= handle_mouse_event(e); break;

            case SDL_EVENT_MOUSE_MOTION:
                result |= handle_mouse_motion_event(e); break;

            case SDL_EVENT_KEY_UP:  break;
            case SDL_EVENT_KEY_DOWN:
                result |= handle_key_event(e); break;

            // Scroll wheel zooms in/out in orbit mode
            case SDL_EVENT_MOUSE_WHEEL:
                if (orbit_mode) {
                    orbit_distance -= e.wheel.y * 2.0f;
                    orbit_distance  = std::clamp(orbit_distance, 35.0f, 500.0f);
                    result |= cg::EventType::REDRAW;
                }
                break;

            default: break;
        }
    }
    return result;
}

//============================================================================
// Scene construction helpers
//============================================================================

/**
 * Construct the sun — an emissive sphere far away in g_sun_direction with
 * a larger transparent halo sphere to fake a glow bloom effect.
 */
std::shared_ptr<cg::SceneNode> construct_sun(int32_t position_loc,
                                              int32_t normal_loc)
{
    const float sun_dist = 1220.0f;
    const float sun_size = 12.0f;

    // Core sphere — fully emissive yellow-white
    auto sun_sphere = std::make_shared<cg::SphereSection>(
        -90.0f, 90.0f, 18, -180.0f, 180.0f, 36, 1.0f,
        position_loc, normal_loc);

    auto sun_material = std::make_shared<cg::PresentationNode>(
        cg::Color4(1.0f, 1.0f,  0.9f, 1.0f),   // ambient
        cg::Color4(1.0f, 1.0f,  0.9f, 1.0f),   // diffuse
        cg::Color4(1.0f, 1.0f,  1.0f, 1.0f),   // specular
        cg::Color4(1.0f, 0.95f, 0.7f, 1.0f),   // emission drives visible brightness
        10.0f);

    auto sun_transform = std::make_shared<cg::TransformNode>();
    sun_transform->translate(
        g_sun_direction.x * sun_dist,
        g_sun_direction.y * sun_dist,
        g_sun_direction.z * sun_dist);
    sun_transform->scale(sun_size, sun_size, sun_size);

    // Halo sphere — transparent, slightly larger, warm emissive color
    auto halo_sphere = std::make_shared<cg::SphereSection>(
        -90.0f, 90.0f, 18, -180.0f, 180.0f, 36, 1.0f,
        position_loc, normal_loc);

    auto halo_material = std::make_shared<cg::PresentationNode>(
        cg::Color4(1.0f, 0.9f, 0.5f, 0.0f),    // ambient
        cg::Color4(1.0f, 0.9f, 0.5f, 0.15f),   // low alpha diffuse
        cg::Color4(0.0f, 0.0f, 0.0f, 0.0f),    // no specular
        cg::Color4(1.0f, 0.85f, 0.4f, 0.2f),   // warm emissive
        1.0f);

    auto halo_transform = std::make_shared<cg::TransformNode>();
    halo_transform->translate(
        g_sun_direction.x * sun_dist,
        g_sun_direction.y * sun_dist,
        g_sun_direction.z * sun_dist);
    halo_transform->scale(sun_size * 2.5f, sun_size * 2.5f, sun_size * 2.5f);

    auto sun_node = std::make_shared<cg::SceneNode>();
    add_sub_tree(sun_node, sun_material,  sun_transform,  sun_sphere);
    add_sub_tree(sun_node, halo_material, halo_transform, halo_sphere);
    return sun_node;
}

/**
 * Construct the ocean water surface — a smooth icosphere at sea level
 * (world radius 30). Land that rises above sea level is naturally occluded
 * by the depth buffer since terrain is drawn first.
 */
std::shared_ptr<cg::SceneNode> construct_water(int32_t position_loc,
                                                int32_t normal_loc)
{
    auto water = std::make_shared<cg::WaterNode>(5, 1.0f,
                                                  position_loc, normal_loc);

    auto water_transform = std::make_shared<cg::TransformNode>();
    water_transform->scale(30.0f, 30.0f, 30.0f);

    auto water_node = std::make_shared<cg::SceneNode>();
    add_sub_tree(water_node, water_transform,
                 std::static_pointer_cast<cg::SceneNode>(water));
    return water_node;
}

/**
 * Construct the animated cloud layer — a smooth icosphere at world radius 33,
 * slightly above sea level. Drawn after terrain and water, before atmosphere.
 */
std::shared_ptr<cg::SceneNode> construct_cloud(int32_t position_loc,
                                                int32_t normal_loc)
{
    auto cloud_mesh = std::make_shared<cg::IcosphereNode>(
        5, 1.0f, position_loc, normal_loc);

    auto cloud_transform = std::make_shared<cg::TransformNode>();
    cloud_transform->scale(33.0f, 33.0f, 33.0f);

    auto cloud_node = std::make_shared<cg::SceneNode>();
    add_sub_tree(cloud_node, cloud_transform,
                 std::static_pointer_cast<cg::SceneNode>(cloud_mesh));
    return cloud_node;
}

/**
 * Construct the spacecraft placeholder geometry — a small sphere using two
 * stacked transform nodes (translate + rotate) so the normal matrix is always
 * derived from a pure rotation, keeping lighting stable during 6DOF maneuvers.
 * Sets g_ship_translate and g_ship_rotate for per-frame updates.
 */
void construct_spacecraft(std::shared_ptr<cg::SceneNode> parent,
                          int32_t position_loc, int32_t normal_loc)
{
    // --- Disc body --- flat grey saucer
    auto disc_mesh = std::make_shared<cg::SphereSection>(
        -90.0f, 90.0f, 6, -180.0f, 180.0f, 24, 1.0f,
        position_loc, normal_loc);

    auto disc_material = std::make_shared<cg::PresentationNode>(
        cg::Color4(0.15f, 0.15f, 0.15f, 1.0f),  // ambient
        cg::Color4(0.45f, 0.45f, 0.45f, 1.0f),  // diffuse — medium grey
        cg::Color4(0.0f,  0.0f,  0.0f,  1.0f),  // specular
        cg::Color4(0.0f,  0.0f,  0.0f,  1.0f),  // no emission
        128.0f);

    // Scale flat: wide in XZ (local right/up plane), thin in Y (forward axis)
    auto disc_transform = std::make_shared<cg::TransformNode>();
    disc_transform->scale(1.2f, 0.25f, 1.2f);
    disc_transform->add_child(disc_material);
    disc_material->add_child(std::static_pointer_cast<cg::SceneNode>(disc_mesh));

    // --- Cockpit sphere --- small yellow dome on top
    auto cockpit_mesh = std::make_shared<cg::SphereSection>(
        -90.0f, 90.0f, 8, -180.0f, 180.0f, 16, 1.0f,
        position_loc, normal_loc);

    auto cockpit_material = std::make_shared<cg::PresentationNode>(
        cg::Color4(0.3f,  0.3f,  0.0f,  1.0f),  // ambient
        cg::Color4(0.8f,  0.75f, 0.1f,  1.0f),  // diffuse — yellow
        cg::Color4(0.0f,  0.0f,  0.0f,  1.0f),  // specular
        cg::Color4(0.1f,  0.1f,  0.0f,  1.0f),  // slight emission — glowing
        64.0f);

    // Position cockpit above disc center, smaller than disc
    auto cockpit_transform = std::make_shared<cg::TransformNode>();
    cockpit_transform->translate(0.0f, 0.1f, 0.22f);
    cockpit_transform->scale(0.45f, 0.25f, 0.45f);
    cockpit_transform->add_child(cockpit_material);
    cockpit_material->add_child(std::static_pointer_cast<cg::SceneNode>(cockpit_mesh));

    // --- Assemble ---
    g_ship_translate = std::make_shared<cg::TransformNode>();
    g_ship_rotate    = std::make_shared<cg::TransformNode>();

    g_ship_rotate->add_child(disc_transform);
    g_ship_rotate->add_child(cockpit_transform);
    g_ship_translate->add_child(g_ship_rotate);

    auto ship_node = std::make_shared<cg::SceneNode>();
    ship_node->add_child(g_ship_translate);
    parent->add_child(ship_node);
}

//============================================================================
// Lighting helpers
//============================================================================

/**
 * Set up fixed scene lighting used by the base LightingShaderNode.
 * Creates a fill point light, a directional light, and the Spotlight
 * (camera-attached, used as scene graph parent for planet/water/cloud shaders).
 */
void construct_lighting(std::shared_ptr<cg::LightingShaderNode> lighting)
{
    lighting->set_global_ambient(cg::Color4(0.4f, 0.4f, 0.4f, 1.0f));

    auto light_0 = std::make_shared<cg::LightNode>(0);
    light_0->set_diffuse(cg::Color4(0.5f, 0.5f, 0.5f, 1.0f));
    light_0->set_specular(cg::Color4(0.5f, 0.5f, 0.5f, 1.0f));
    light_0->set_position(cg::HPoint3(1000.0f, 0.0f, 0.0f, 1.0f));
    light_0->enable();

    auto light_1 = std::make_shared<cg::LightNode>(1);
    light_1->set_diffuse(cg::Color4(0.7f, 0.7f, 0.7f, 1.0f));
    light_1->set_specular(cg::Color4(0.7f, 0.7f, 0.7f, 1.0f));
    light_1->set_position(cg::HPoint3(1000.0f, 0.0f, 0.0f, 0.0f));
    light_1->enable();

    // Spotlight is used as the scene graph parent for planet/water/cloud/moon/ship
    // shader branches. It is not enabled as an actual light source.
    Spotlight = std::make_shared<cg::LightNode>(2);
    cg::Point3  pos = g_camera->get_position();
    cg::Vector3 dir = g_camera->get_view_plane_normal() * -1.0f;
    Spotlight->set_position(cg::HPoint3(pos.x, pos.y, pos.z, 1.0f));
    Spotlight->set_spotlight(dir, 32.0f, 30.0f);

    g_camera->add_child(light_0);
    light_0->add_child(light_1);
    light_1->add_child(Spotlight);
}

/**
 * Construct sun lighting for the planet shader.
 * Light 0: soft fill point light to prevent completely black shadows.
 * Light 1: directional sun — drives primary diffuse and specular.
 * @return  Returns light_1 so geometry can be attached beneath it.
 */
std::shared_ptr<cg::LightNode> construct_planet_lighting(
    std::shared_ptr<cg::PlanetShaderNode> planet_shader)
{
    auto light_0 = std::make_shared<cg::LightNode>(0);
    light_0->set_diffuse(cg::Color4(0.2f, 0.2f, 0.2f, 1.0f));
    light_0->set_specular(cg::Color4(0.1f, 0.1f, 0.1f, 1.0f));
    light_0->set_position(cg::HPoint3(75.0f, 75.0f, 30.0f, 1.0f));
    light_0->enable();

    auto light_1 = std::make_shared<cg::LightNode>(1);
    light_1->set_diffuse(cg::Color4(0.9f, 0.9f, 0.9f, 1.0f));
    light_1->set_specular(cg::Color4(0.9f, 0.9f, 0.9f, 1.0f));
    light_1->set_position(cg::HPoint3(
        g_sun_direction.x, g_sun_direction.y, g_sun_direction.z, 0.0f));
    light_1->enable();

    planet_shader->add_child(light_0);
    light_0->add_child(light_1);
    return light_1;
}

/**
 * Construct sun lighting for the water shader.
 * Uses a single directional sun light with white specular for sharp glints.
 * @return  Returns the last light node so geometry can be attached beneath it.
 */
std::shared_ptr<cg::LightNode> construct_water_lighting(
    std::shared_ptr<cg::WaterShaderNode> water_shader)
{
    auto light_1 = std::make_shared<cg::LightNode>(1);
    light_1->set_diffuse(cg::Color4(0.7f, 0.7f, 0.7f, 1.0f));
    light_1->set_specular(cg::Color4(1.0f, 1.0f, 1.0f, 1.0f));  // white for glints
    light_1->set_position(cg::HPoint3(
        g_sun_direction.x, g_sun_direction.y, g_sun_direction.z, 0.0f));
    light_1->enable();

    water_shader->add_child(light_1);
    return light_1;
}

/**
 * Template lighting helper for shader nodes that use the standard
 * two-light sun setup (fill + directional sun). Used for clouds, moon,
 * atmosphere, and ship shaders.
 * @return  Returns light_1 so geometry can be attached beneath it.
 */
template<typename T>
std::shared_ptr<cg::LightNode> construct_sun_lighting(
    std::shared_ptr<T> shader)
{
    auto light_0 = std::make_shared<cg::LightNode>(0);
    light_0->set_diffuse(cg::Color4(0.2f, 0.2f, 0.2f, 1.0f));
    light_0->set_specular(cg::Color4(0.1f, 0.1f, 0.1f, 1.0f));
    light_0->set_position(cg::HPoint3(75.0f, 75.0f, 30.0f, 1.0f));
    light_0->enable();

    auto light_1 = std::make_shared<cg::LightNode>(1);
    light_1->set_diffuse(cg::Color4(0.9f, 0.9f, 0.9f, 1.0f));
    light_1->set_specular(cg::Color4(1.0f, 1.0f, 1.0f, 1.0f));
    light_1->set_position(cg::HPoint3(
        g_sun_direction.x, g_sun_direction.y, g_sun_direction.z, 0.0f));
    light_1->enable();

    shader->add_child(light_0);
    light_0->add_child(light_1);
    return light_1;
}

//============================================================================
// Skybox construction
//============================================================================

/**
 * Load and initialize the skybox cubemap.
 * Cubemap textures generated with the Space-3D tool at https://tools.wwwtyro.net/
 */
void construct_skybox()
{
    g_skybox_shader = std::make_shared<cg::SkyboxShaderNode>();
    if (!g_skybox_shader->create("SampleProject/skybox_shader.vert",
                                 "SampleProject/skybox_shader.frag") ||
        !g_skybox_shader->get_locations())
    {
        std::cerr << "Failed to create skybox shader\n";
        exit(-1);
    }

    g_skybox = std::make_shared<cg::Skybox>();

    // Note: top and bottom faces are swapped to match the cubemap convention
    // of the Space-3D generator tool.
    std::vector<std::string> faces = {
        "textures/skybox/right.png",
        "textures/skybox/left.png",
        "textures/skybox/bottom.png",
        "textures/skybox/top.png",
        "textures/skybox/front.png",
        "textures/skybox/back.png"
    };

    if (!g_skybox->load_cubemap(faces))
    {
        std::cerr << "Failed to load skybox textures\n";
        exit(-1);
    }

    g_skybox->initialize(g_skybox_shader->get_position_loc());
    g_skybox_shader->add_child(g_skybox);
}

//============================================================================
// construct_scene — builds the full scene graph
//
// Draw order (important for correct transparency and depth):
//   1. Skybox              — drawn first, no depth write
//   2. Planet terrain      — opaque, writes depth
//   3. Moon                — opaque, writes depth
//   4. Water               — slightly transparent, alpha blend
//   5. Clouds              — transparent, alpha blend, depth mask off
//   6. Atmosphere          — transparent, drawn last, depth mask off
//   7. Sun + spacecraft    — under base lighting shader
//============================================================================

void construct_scene()
{
    // Base lighting shader — used for sun sphere and spacecraft
    auto shader = std::make_shared<cg::LightingShaderNode>();
    if(!shader->create("SampleProject/pixel_lighting_tex.vert",
                       "SampleProject/pixel_lighting_tex.frag") ||
       !shader->get_locations())
        exit(-1);

    int32_t position_loc = shader->get_position_loc();
    int32_t normal_loc   = shader->get_normal_loc();

    // Camera — perspective projection, looking at planet origin
    g_camera = std::make_shared<cg::CameraNode>();
    g_camera->set_position(cg::Point3(0.0f, -100.0f, 0.0f));
    g_camera->set_look_at_pt(cg::Point3(0.0f, 0.0f, 0.0f));
    g_camera->set_view_up(cg::Vector3(0.0f, 0.0f, 1.0f));
    g_camera->set_perspective(50.0f, 1.0f, 1.0f, 2000.0f);

    construct_lighting(shader);

    // --- Planet ---
    // Subdivision 6 icosphere (~40k triangles) with fBm noise displacement.
    // Elevation coloring is computed in the fragment shader from the noise value.
    auto planet_shader = std::make_shared<cg::PlanetShaderNode>();
    if (!planet_shader->create("SampleProject/planet.vert",
                               "SampleProject/planet.frag") ||
        !planet_shader->get_locations())
        exit(-1);

    planet_shader->set_global_ambient(cg::Color4(0.4f, 0.4f, 0.4f, 1.0f));
    planet_shader->set_noise_params(0.55f, 3.0f);
    planet_shader->set_elevation_color(true);

    int32_t planet_pos_loc = planet_shader->get_position_loc();
    int32_t planet_nrm_loc = planet_shader->get_normal_loc();

    auto icosphere = std::make_shared<cg::IcosphereNode>(
        6, 1.0f, planet_pos_loc, planet_nrm_loc);

    auto icosphere_material = std::make_shared<cg::PresentationNode>(
        cg::Color4(0.15f, 0.15f, 0.15f),
        cg::Color4(0.5f,  0.5f,  0.5f),
        cg::Color4(1.0f,  1.0f,  1.0f),
        cg::Color4(0.0f,  0.0f,  0.0f),
        85.0f);
    auto icosphere_transform = std::make_shared<cg::TransformNode>();
    icosphere_transform->scale(30.0f, 30.0f, 30.0f);
    auto planet = std::make_shared<cg::SceneNode>();
    add_sub_tree(planet, icosphere_material, icosphere_transform,
                 std::static_pointer_cast<cg::SceneNode>(icosphere));
    
    // --- Water ---
    g_water_shader = std::make_shared<cg::WaterShaderNode>();
    if (!g_water_shader->create("SampleProject/water.vert",
                                "SampleProject/water.frag") ||
        !g_water_shader->get_locations())
        exit(-1);

    g_water_shader->set_global_ambient(cg::Color4(0.4f, 0.4f, 0.4f, 1.0f));
    g_water_shader->set_water_params(
        g_sun_direction,
        cg::Color4(0.05f, 0.2f,  0.4f,  1.0f),  // shallow — blue-green
        cg::Color4(0.0f,  0.02f, 0.08f, 1.0f),  // deep — dark navy
        2048.0f,                                // shininess — sharp sun glints
        cg::Vector3(0.2f, 0.4f, 0.7f),          // sky color for Fresnel
        0.6f                                    // Fresnel strength
    );

    int32_t water_pos_loc = g_water_shader->get_position_loc();
    int32_t water_nrm_loc = g_water_shader->get_normal_loc();
    auto water = construct_water(water_pos_loc, water_nrm_loc);

    // --- Clouds ---
    g_cloud_shader = std::make_shared<cg::CloudShaderNode>();
    if (!g_cloud_shader->create("SampleProject/cloud.vert",
                                "SampleProject/cloud.frag") ||
        !g_cloud_shader->get_locations())
        exit(-1);

    g_cloud_shader->set_global_ambient(cg::Color4(0.4f, 0.4f, 0.4f, 1.0f));
    g_cloud_shader->set_cloud_params(
        g_sun_direction,
        0.05f,                           // threshold — lower = more coverage
        0.15f,                           // softness — feathered cloud edges
        0.85f,                           // max opacity
        cg::Vector3(1.0f, 0.3f, 0.1f),   // drift direction
        0.02f                            // drift speed
    );

    int32_t cloud_pos_loc = g_cloud_shader->get_position_loc();
    int32_t cloud_nrm_loc = g_cloud_shader->get_normal_loc();
    auto clouds = construct_cloud(cloud_pos_loc, cloud_nrm_loc);

    // --- Atmosphere ---
    // Ray-marched Rayleigh/Mie scattering on an icosphere shell.
    // Shell radius (42) matches the atmosphere_radius uniform in the shader.
    // planet_radius (20) anchors the density falloff — not the visual surface.
    g_atmo_shader = std::make_shared<cg::AtmosphereShaderNode>();
    if (!g_atmo_shader->create("SampleProject/atmosphere.vert",
                               "SampleProject/atmosphere.frag") ||
        !g_atmo_shader->get_locations())
        exit(-1);

    g_atmo_shader->set_params(20.0f, 42.0f, g_sun_direction, 20.0f);

    int32_t atmo_pos_loc = g_atmo_shader->get_position_loc();
    int32_t atmo_nrm_loc = g_atmo_shader->get_normal_loc();

    auto atmo_mesh = std::make_shared<cg::IcosphereNode>(
        5, 1.0f, atmo_pos_loc, atmo_nrm_loc);
    auto atmo_transform = std::make_shared<cg::TransformNode>();
    atmo_transform->scale(42.0f, 42.0f, 42.0f);
    auto atmo_node = std::make_shared<cg::SceneNode>();
    add_sub_tree(atmo_node, atmo_transform, std::static_pointer_cast<cg::SceneNode>(atmo_mesh));
    
    // --- Moon ---
    // Reuses the planet shader with different noise params and no elevation
    // coloring — the grey material color drives the visual appearance.
    auto moon_shader = std::make_shared<cg::PlanetShaderNode>();
    if (!moon_shader->create("SampleProject/planet.vert",
                             "SampleProject/planet.frag") ||
        !moon_shader->get_locations())
        exit(-1);

    moon_shader->set_global_ambient(cg::Color4(0.3f, 0.3f, 0.3f, 1.0f));
    moon_shader->set_noise_params(0.3f, 5.0f);   // rougher, more cratered
    moon_shader->set_elevation_color(false);     // grey material drives appearance

    int32_t moon_pos_loc = moon_shader->get_position_loc();
    int32_t moon_nrm_loc = moon_shader->get_normal_loc();

    auto moon_icosphere = std::make_shared<cg::IcosphereNode>(
        5, 1.0f, moon_pos_loc, moon_nrm_loc);

    auto moon_material = std::make_shared<cg::PresentationNode>(
        cg::Color4(0.1f, 0.1f, 0.1f, 1.0f),
        cg::Color4(0.5f, 0.5f, 0.5f, 1.0f),
        cg::Color4(0.2f, 0.2f, 0.2f, 1.0f),
        cg::Color4(0.0f, 0.0f, 0.0f, 1.0f),
        10.0f);
    auto moon_scale = std::make_shared<cg::TransformNode>();
    moon_scale->scale(10.0f, 10.0f, 10.0f);
    g_moon_transform = std::make_shared<cg::TransformNode>();
    auto moon_node = std::make_shared<cg::SceneNode>();
    add_sub_tree(moon_node, moon_scale, moon_material,
                 std::static_pointer_cast<cg::SceneNode>(moon_icosphere));
    g_moon_transform->add_child(moon_node);

    // --- Scene graph assembly ---
    g_scene_root = std::make_shared<cg::SceneNode>();

    // Skybox draws first — no depth write, always behind everything
    construct_skybox();
    g_scene_root->add_child(g_skybox_shader);

    // Base lighting shader — camera, lights, Spotlight as scene graph parent
    g_scene_root->add_child(shader);
    shader->add_child(g_camera);

    // Sun and spacecraft hang under the base shader via myscene
    auto myscene = std::make_shared<cg::SceneNode>();
    Spotlight->add_child(myscene);
    myscene->add_child(construct_sun(position_loc, normal_loc));

    // Planet — own shader branch with sun lighting
    Spotlight->add_child(planet_shader);
    auto planet_last_light = construct_planet_lighting(planet_shader);
    planet_last_light->add_child(planet);

    // Moon — own shader branch
    Spotlight->add_child(moon_shader);
    auto moon_last_light = construct_sun_lighting(moon_shader);
    moon_last_light->add_child(g_moon_transform);

    // Water — own shader branch, draws after terrain so depth buffer occludes land
    Spotlight->add_child(g_water_shader);
    auto water_last_light = construct_water_lighting(g_water_shader);
    water_last_light->add_child(water);

    // Clouds — own shader branch, alpha blended over water and terrain
    Spotlight->add_child(g_cloud_shader);
    auto cloud_last_light = construct_sun_lighting(g_cloud_shader);
    cloud_last_light->add_child(clouds);

    // Atmosphere — drawn last, sphere shell depth-tested against everything above
    Spotlight->add_child(g_atmo_shader);
    g_atmo_shader->add_child(atmo_node);

    // Spacecraft — own shader branch with sun lighting so it is not affected
    // by the camera-following Spotlight
    auto ship_shader = std::make_shared<cg::LightingShaderNode>();
    if (!ship_shader->create("SampleProject/pixel_lighting_tex.vert",
                             "SampleProject/pixel_lighting_tex.frag") ||
        !ship_shader->get_locations())
        exit(-1);

    ship_shader->set_global_ambient(cg::Color4(0.4f, 0.4f, 0.4f, 1.0f));

    auto ship_parent = std::make_shared<cg::SceneNode>();
    construct_spacecraft(ship_parent,
                         ship_shader->get_position_loc(),
                         ship_shader->get_normal_loc());

    Spotlight->add_child(ship_shader);
    auto ship_last_light = construct_sun_lighting(ship_shader);
    ship_last_light->add_child(ship_parent);
}

//============================================================================
// main
//============================================================================

int main(int argc, char **argv)
{
    cg::set_root_paths(argv[0]);
    cg::init_logging("SampleProject.log");

    // Keyboard controls summary
    std::cout << "============================================================\n";
    std::cout << "  Procedural Planet Viewer — Bryan Garry  (605.767)\n";
    std::cout << "============================================================\n";
    std::cout << "  ESC          Exit\n";
    std::cout << "  R            Reset camera\n";
    std::cout << "\n";
    std::cout << "  TAB          Toggle orbit mode\n";
    std::cout << "    Mouse drag   Rotate orbit azimuth / elevation\n";
    std::cout << "    Scroll       Zoom in / out\n";
    std::cout << "\n";
    std::cout << "  SPACE        Toggle spacecraft mode\n";
    std::cout << "    W / S        Pitch up / down\n";
    std::cout << "    A / D        Yaw left / right\n";
    std::cout << "    Q / E        Roll left / right\n";
    std::cout << "    I / K        Thrust forward / backward\n";
    std::cout << "    J / L        Strafe left / right\n";
    std::cout << "    U / O        Strafe up / down\n";
    std::cout << "    C            Toggle first / third person camera\n";
    std::cout << "============================================================\n";

    // Initialize SDL
    if(!SDL_Init(SDL_INIT_VIDEO))
    {
        std::cout << "Error initializing SDL: " << SDL_GetError() << '\n';
        exit(1);
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,      1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS,1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES,4);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,        24);

    SDL_PropertiesID props = SDL_CreateProperties();
    if(props == 0)
    {
        std::cout << "Error creating SDL properties: " << SDL_GetError() << '\n';
        exit(1);
    }

    SDL_SetStringProperty(
        props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, "Final Project - Bryan Garry");
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_OPENGL_BOOLEAN,    true);
    SDL_SetNumberProperty( props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER,       800);
    SDL_SetNumberProperty( props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER,      600);
    SDL_SetNumberProperty( props, SDL_PROP_WINDOW_CREATE_X_NUMBER,           100);
    SDL_SetNumberProperty( props, SDL_PROP_WINDOW_CREATE_Y_NUMBER,           100);

    g_sdl_window = SDL_CreateWindowWithProperties(props);
    if(g_sdl_window == nullptr)
    {
        std::cout << "Error creating SDL window: " << SDL_GetError() << '\n';
        exit(1);
    }

    g_gl_context = SDL_GL_CreateContext(g_sdl_window);

    std::cout << "OpenGL  " << glGetString(GL_VERSION) << ", GLSL "
              << glGetString(GL_SHADING_LANGUAGE_VERSION) << '\n';

#if BUILD_WINDOWS
    int32_t glew_init_result = glewInit();
    if(GLEW_OK != glew_init_result)
    {
        std::cout << "GLEW Error: " << glewGetErrorString(glew_init_result) << '\n';
        exit(EXIT_FAILURE);
    }
#endif

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    construct_scene();

    glEnable(GL_MULTISAMPLE);
    glEnable(GL_DEPTH_TEST);
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);

    reshape(800, 600);
    update_view(g_mouse_x, g_mouse_y, g_forward);
    display();

    // Main loop
    cg::EventType event_result = cg::EventType::NONE;
    while(true)
    {
        // Delta time for physics and animation
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> elapsed = now - g_last_time;
        g_last_time = now;
        g_time     += elapsed.count();

        event_result = handle_events();
        if(event_result & cg::EventType::EXIT) break;

        // Per-frame updates — always run regardless of input mode
        g_cloud_shader->set_time(g_time);
        update_moon_orbit();

        // Mode-specific camera and physics updates
        if (spacecraft_mode) {
            g_spacecraft.update(elapsed.count(), g_planet_body, g_moon_body);
            update_ship_transform();
            update_spacecraft_camera();
        } else if (orbit_mode) {
            update_orbit_camera();
        } else if (g_animate) {
            update_view(g_mouse_x, g_mouse_y, g_forward);
        }

        update_spotlight();
        display();
        sleep(DRAW_INTERVAL_MILLIS);
    }

    SDL_GL_DestroyContext(g_gl_context);
    SDL_DestroyWindow(g_sdl_window);
    SDL_Quit();
    return 0;
}
