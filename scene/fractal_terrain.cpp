//
//  fractal_terrain.cpp
//  JHU_605.667
//
//  Created by Bryan Garry.
//

#include "scene/fractal_terrain.hpp"
#include <cmath>
#include <random>
#include <algorithm>
#include <iostream>

namespace cg
{

FractalTerrain::FractalTerrain(uint32_t n,
                               float    world_size,
                               float    height_scale,
                               float    roughness,
                               uint32_t seed,
                               int32_t  position_loc,
                               int32_t  normal_loc)
{
    // Grid is (2^n + 1) x (2^n + 1)
    dim_ = (1u << n) + 1;
    uint32_t grid_size = dim_ * dim_;

    // Initialize height map to -1
    height_.assign(grid_size, -1.0f);

    // Run Diamond-Square to populate height_
    diamond_square(height_scale, roughness, seed);

    // Build vertex list — one vertex per grid point
    // Grid spans [-world_size/2, world_size/2] in X-Y plane, [0, height_scale] along Z axis
    float half = world_size * 0.5f;
    float step = world_size / static_cast<float>(dim_ - 1);

    vertices_.reserve(grid_size);
    for(uint32_t row = 0; row < dim_; ++row)
    {
        for(uint32_t col = 0; col < dim_; ++col)
        {
            VertexAndNormal vtx;
            vtx.vertex.x = -half + col * step;
            vtx.vertex.y = -half + row * step;
            vtx.vertex.z = height_[row * dim_ + col];
            vtx.normal = {0.0f, 0.0f, 1.0f}; // placeholder until end() recalculates
            vertices_.push_back(vtx);
        }
    }

    // Build face list using tri_surface helper
    construct_row_col_face_list(dim_, dim_);

    // Calculate vertex normals and upload to GPU
    end(position_loc, normal_loc);

    // Print to console terraind details
    std::cout << "[FractalTerrain] Generated " << dim_ << "x" << dim_
              << " grid (" << faces_.size() / 3 << " triangles)\n";
}

// Diamond-Square algorithm implementation
void FractalTerrain::diamond_square(float height_scale, float roughness, uint32_t seed)
{
    std::mt19937 rng(seed);

    // Seed the 4 corners at Z=0
    auto idx = [&](int r, int c) { return r * (int)dim_ + c; };
    int last = (int)dim_ - 1;

    height_[idx(0, 0)] = 0.0f;
    height_[idx(0, last)] = 0.0f;
    height_[idx(last, 0)] = 0.0f;
    height_[idx(last, last)] = 0.0f;

    // Random range starts at height_scale and shrinks by roughness each iteration
    float range = height_scale;

    // Step size starts at full grid and halves each iteration
    int step = last;

    while(step > 1)
    {
        int half = step / 2;
        std::uniform_real_distribution<float> dist(-range, range);

        // ---- Diamond step ----
        // For each square, compute midpoint = avg of 4 corners + random offset
        for (int r = 0; r < last; r += step)
        {
            for (int c = 0; c < last; c += step)
            {
                float avg = (height_[idx(r, c)] +
                             height_[idx(r, c + step)] +
                             height_[idx(r + step, c)] +
                             height_[idx(r + step, c + step)]) * 0.25f;

                float val = avg + dist(rng);
                // Clamp to >= 0 so terrain only rises from ground plane
                height_[idx(r + half, c + half)] = std::max(0.0f, val);
            }
        }

        // ---- Square step ----
        // For each diamond, compute edge midpoints = avg of available neighbors + random
        for(int r = 0; r <= last; r += half)
        {
            for(int c = (r / half % 2 == 0) ? half : 0; c <= last; c += step)
            {
                float sum = 0.0f;
                int count = 0;

                // Up
                if (r - half >= 0)
                {
                    sum += height_[idx(r - half, c)];
                    ++count;
                }
                // Down
                if (r + half <= last)
                {
                    sum += height_[idx(r + half, c)];
                    ++count;
                }
                // Left
                if (c - half >= 0)
                {
                    sum += height_[idx(r, c - half)];
                    ++count;
                }
                // Right
                if (c + half <= last)
                {
                    sum += height_[idx(r, c + half)];
                    ++count;
                }

                float val = (sum / count) + dist(rng);
                height_[idx(r, c)] = std::max(0.0f, val);
            }
        }

        // Reduce random range and step size for next iteration
        range *= roughness;
        step = half;
    }
}

float FractalTerrain::get_height(int row, int col) const
{
    row = std::max(0, std::min((int)dim_ - 1, row));
    col = std::max(0, std::min((int)dim_ - 1, col));
    return height_[row * dim_ + col];
}

void FractalTerrain::set_height(int row, int col, float value)
{
    int i = row * (int)dim_ + col;
    if(height_[i] < 0.0f)
    {
        height_[i] = value;
    }
}

float FractalTerrain::get_height_at(float x, float y, float world_size) const
{
    // Error checking to make sure terrain map has been initialized before trying to access it
    if (dim_ == 0 || height_.empty())
    {
        std::cerr << "get_height_at: invalid state dim_=" << dim_ << " height_.size()=" << height_.size() << "\n";
        return 0.0f;
    }
    
    float half = world_size * 0.5f;
    float step = world_size / static_cast<float>(dim_ - 1);

    // Convert world coordinates to grid float coordinates
    float gc = (x + half) / step;
    float gr = (y + half) / step;

    // Clamp to grid bounds
    gc = std::max(0.0f, std::min((float)(dim_ - 2), gc));
    gr = std::max(0.0f, std::min((float)(dim_ - 2), gr));

    int c0 = (int)gc;
    int r0 = (int)gr;
    float fc = gc - c0;
    float fr = gr - r0;

    // Bilinear interpolation across the grid cell
    float h00 = get_height(r0, c0);
    float h10 = get_height(r0, c0 + 1);
    float h01 = get_height(r0 + 1, c0);
    float h11 = get_height(r0 + 1, c0 + 1);

    // Return average cell height
    return (h00 * (1-fc) * (1-fr)
            + h10 * fc * (1-fr)
            + h01 * (1-fc) * fr
            + h11 * fc * fr);
}

} // namespace cg
