//
//  fractal_terrain.hpp
//  JHU_605.667
//
//  Created by Bryan Garry.
//
//  Uses Diamond-Square Algorithm from Lecture Notes
//      - Grid size is (2^n + 1) x (2^n + 1)
//      - Corners seeded at Z = 0
//      - Diamond step: midpoint of each square = avg of 4 corners + random offset
//      - Square step:  midpoint of each diamond = avg of neighbors + random offset
//      - Random range halved each iteration, scaled by roughness factor
//      - All Z values clamped to >= 0 so terrain only rises from ground plane

#ifndef __SCENE_FRACTAL_TERRAIN_HPP__
#define __SCENE_FRACTAL_TERRAIN_HPP__

#include "scene/tri_surface.hpp"

namespace cg
{

class FractalTerrain : public TriSurface
{
  public:
    /**
     * Construct a fractal terrain mesh using the Diamond-Square algorithm.
     *
     * @param  n   Grid resolution exponent. Grid = (2^n + 1) x (2^n + 1).
     * @param  world_size   XY extent of the terrain in world units (centered at origin).
     * @param  height_scale     Maximum height of terrain features in world units.
     * @param  roughness      Controls how fast random range decreases each iteration.
     *                     0.5 = smooth rolling hills, 1.0 = Brownian motion (rocky).
     * @param  seed     Random seed for deterministic terrain generation.
     * @param  position_loc  Shader position attribute location.
     * @param  normal_loc    Shader normal attribute location.
     */
    FractalTerrain(uint32_t n,
                   float    world_size,
                   float    height_scale,
                   float    roughness,
                   uint32_t seed,
                   int32_t  position_loc,
                   int32_t  normal_loc);
    
    /**
     * Returns the interpolated terrain height at world-space (x, y).
     * @param  world_size  Must match the world_size used at construction.
     */
    float get_height_at(float x, float y, float world_size) const;
    
    /**
     * Run the Diamond-Square algorithm to fill height_.
     * @param  height_scale     Scale applied to random displacements.
     * @param  roughness     Offset factor for random range per iteration.
     * @param  seed          RNG seed.
     */
    void diamond_square(float height_scale, float roughness, uint32_t seed);

    
    // Get height at (row, col), clamping indices to [0, dim_-1]
    float get_height(int row, int col) const;
    
    // Set height at (row, col) only if not yet assigned
    void set_height(int row, int col, float value);

  private:
    // Grid dimension = 2^n + 1
    uint32_t dim_;

    // 2D height array
    std::vector<float> height_;
};

} // namespace cg

#endif
