#pragma once

struct ConfigBundle
{
    struct SliceConfig
    {
        bool use_slice{ false };
        bool slice_x{ true };
        bool slice_y{ false };
        bool slice_z{ false };
        
        enum Policy { RemoveGreaterThan, RemoveLessThan, RemoveEqual, RemoveNonEqual };
        bool slice_mesh{ false };
        bool slice_pc{ false };
        Policy remove_policy{ RemoveGreaterThan };
    } slice_config;

    struct RenderConfig
    {
        bool cull_face{ true };
        bool draw_face{ true };
        struct Material
        {
            float ambient{ 0.3f };
            float diffuse{ 0.8f };
            float specular{ 0.5f };
            float shininess{ 32.0f };
        } material;

        bool mesh_inner_visible{ true };
        bool mesh_outer_visible{ true };
        bool point_cloud_visible{ true };
        bool slice_visible{ true };
        float mesh_inner_alpha{ 1.0f };
        float mesh_outer_alpha{ 1.0f };
        float point_cloud_alpha{ 1.0f };
        float slice_alpha{ 0.5f };
    } render_config;
};