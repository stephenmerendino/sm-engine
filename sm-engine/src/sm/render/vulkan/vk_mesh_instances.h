#pragma once

#include "sm/core/types.h"
#include "sm/math/mat44.h"

namespace sm
{
    struct arena_t;
    struct mesh_t;
    struct material_t;
    struct string_t;

    using mesh_instance_id_t = u32;
    extern const mesh_instance_id_t INVALID_MESH_INSTANCE_ID;
    extern const u32 INVALID_MESH_INSTANCE_INDEX;

    struct transform_t
    {
        //vec3_t scale { .x = 1.0f, .y = 1.0f, .z = 1.0f };
        //vec4_t quaternion { .x = 0.0f, .y = 0.0f, .z = 0.0f, .w = 1.0f };
        //vec3_t translation { .x = 0.0f, .y = 0.0f, .z = 0.0f };
        mat44_t model = mat44_t::IDENTITY;
    };

    struct push_constants_t
    {
        size_t size = 0;
        void* data = nullptr;
    };

    enum class mesh_instance_flags_t : u32
    {
        NONE     = 0x00000000,
        IS_DEBUG = 0x00000001
    };

    struct mesh_instances_t
    {
        mesh_instance_id_t* ids             = nullptr;
        mesh_t** meshes                     = nullptr;
        material_t** materials              = nullptr;
        push_constants_t* push_constants    = nullptr;
        transform_t* transforms             = nullptr;
        u32* flags                          = nullptr;
        size_t capacity                     = 0;
    };

    void mesh_instances_init(arena_t* arena, mesh_instances_t* mesh_instances, size_t capacity);
    u32 mesh_instances_get_index(mesh_instances_t* mesh_instances, mesh_instance_id_t id);
    mesh_instance_id_t mesh_instances_add(mesh_instances_t* mesh_instances, mesh_t* mesh, material_t* material, const push_constants_t& push_constants, const transform_t& initial_transform, u32 flags);
    void mesh_instances_append(mesh_instances_t* dst, mesh_instances_t* src);

    void mesh_instances_names_init();
    void mesh_instances_register_name(mesh_instance_id_t id, string_t* name);
    string_t* mesh_instances_look_up_name(mesh_instance_id_t id);
};
