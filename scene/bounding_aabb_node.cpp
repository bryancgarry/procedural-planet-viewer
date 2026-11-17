#include "scene/bounding_aabb_node.hpp"
#include <cmath>
#include <iostream>

namespace cg
{

AABBNode::AABBNode() : BoundingNode() {}

AABBNode::~AABBNode() {}

void AABBNode::set(const Point3 &min_pt, const Point3 &max_pt) { box_.update(min_pt, max_pt); }

void AABBNode::set_name(const std::string &name) { name_ = name; }

void AABBNode::draw(SceneState& scene_state)
{
    // Skip frustum test if the frustum hasn't been built yet
    if (!scene_state.frustum_valid)
    {
        SceneNode::draw(scene_state);
        return;
    }

    // Skip test if an ancestor was already confirmed fully inside, all descendants are guaranteed visible as well
    if (scene_state.fully_inside)
    {
        SceneNode::draw(scene_state);
        return;
    }

    FrustumIntersectType result = scene_state.view_frustum.intersect(box_);

    if (result == FrustumIntersectType::OUTSIDE)
    {
        if (!last_culled_)
        {
            std::cout << "[FrustumCull] \"" << name_ << "\" CULLED\n";
            last_culled_ = true;
        }
        return;
    }

    if (last_culled_)
    {
        std::cout << "[FrustumCull] \"" << name_ << "\" VISIBLE\n";
        last_culled_ = false;
    }

    if (result == FrustumIntersectType::INSIDE)
    {
        // Fully inside, descendants can skip further frustum tests
        bool prev = scene_state.fully_inside;
        scene_state.fully_inside = true;
        SceneNode::draw(scene_state);
        scene_state.fully_inside = prev;
    }
    else
    {
        // Intersects a plane, children still need individual tests
        SceneNode::draw(scene_state);
    }
}

void AABBNode::update(SceneState &scene_state) { SceneNode::update(scene_state); }

void AABBNode::find_closest_intersect(Ray3 ray, SceneState &current_state, SceneState &closest) {}

bool AABBNode::does_intersect_exist(Ray3 ray, float d, SceneState &current_state) { return false; }

} // namespace cg
