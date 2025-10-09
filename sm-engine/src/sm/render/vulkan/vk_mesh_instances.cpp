#include "sm/render/vulkan/vk_mesh_instances.h"
#include "sm/memory/arena.h"

using namespace sm;

const mesh_instance_id_t sm::INVALID_MESH_INSTANCE_ID = UINT32_MAX;
const u32 sm::INVALID_MESH_INSTANCE_INDEX = UINT32_MAX;

struct mesh_instance_name_registry_t
{
    static const size_t MAX_NUM_REGISTERED_MESH_INSTANCES = 4096;
    mesh_instance_id_t ids[MAX_NUM_REGISTERED_MESH_INSTANCES];
    string_t* names[MAX_NUM_REGISTERED_MESH_INSTANCES];
};

static mesh_instance_name_registry_t s_mesh_instance_registry;

static u32 mesh_instance_id_provision()
{
    static mesh_instance_id_t s_next_mesh_instance_id = 0;
    return s_next_mesh_instance_id++;
}

void sm::mesh_instances_init(arena_t* arena, mesh_instances_t* mesh_instances, size_t capacity)
{
    mesh_instances->ids = (mesh_instance_id_t*)arena_alloc_array(arena, mesh_instance_id_t, capacity);
    for (int i = 0; i < capacity; i++)
    {
        mesh_instances->ids[i] = INVALID_MESH_INSTANCE_ID;
    }
    mesh_instances->flags = (u32*)arena_alloc_array_zero(arena, mesh_instance_flags_t, capacity);
    mesh_instances->meshes = (mesh_t**)arena_alloc_array_zero(arena, mesh_t*, capacity);
    mesh_instances->materials = (material_t**)arena_alloc_array_zero(arena, material_t*, capacity);
    mesh_instances->push_constants = (push_constants_t*)arena_alloc_array_zero(arena, push_constants_t, capacity);
    mesh_instances->transforms = (transform_t*)arena_alloc_array_zero(arena, transform_t, capacity);
    mesh_instances->capacity = capacity;
}

u32 sm::mesh_instances_get_index(mesh_instances_t* mesh_instances, mesh_instance_id_t id)
{
    for (int i = 0; i < mesh_instances->capacity; i++)
    {
        if (mesh_instances->ids[i] == id)
        {
            return i;
        }
    }

    return INVALID_MESH_INSTANCE_INDEX;
}

mesh_instance_id_t sm::mesh_instances_add(mesh_instances_t* mesh_instances, mesh_t* mesh, material_t* material, const push_constants_t& push_constants, const transform_t& initial_transform, u32 flags)
{
    // loop through mesh instances ids until you find an empty slot
    int slot = -1;
    for(int i = 0; i < mesh_instances->capacity; i++)
    {
        if (mesh_instances->ids[i] == sm::INVALID_MESH_INSTANCE_ID)
        {
            slot = i;
            break;
        }
    }

    SM_ASSERT(slot != -1);

    mesh_instances->ids[slot] = mesh_instance_id_provision();
    mesh_instances->meshes[slot] = mesh;
    mesh_instances->materials[slot] = material;
    mesh_instances->push_constants[slot] = push_constants;
    mesh_instances->transforms[slot] = initial_transform;
    mesh_instances->flags[slot] = flags;

    return mesh_instances->ids[slot];
}

void sm::mesh_instances_append(mesh_instances_t* dst, mesh_instances_t* src)
{
    int start_dst_search_index = 0;

    for(int i = 0; i < src->capacity; i++)
    {
        if(src->ids[i] == INVALID_MESH_INSTANCE_ID)
        {
            continue;
        }

        for(int j = start_dst_search_index; j < dst->capacity; j++)
        {
            if(dst->ids[j] != INVALID_MESH_INSTANCE_ID)
            {
                continue;
            }

            dst->ids[j] = src->ids[i];
            dst->flags[j] = src->flags[i];
            dst->meshes[j] = src->meshes[i];
            dst->materials[j] = src->materials[i];
            dst->push_constants[j] = src->push_constants[i];
            dst->transforms[j] = src->transforms[i];
            start_dst_search_index = j + 1;
            break;
        }
    }
}

void sm::mesh_instances_names_init()
{
    for (int i = 0; i < mesh_instance_name_registry_t::MAX_NUM_REGISTERED_MESH_INSTANCES; i++)
    {
        s_mesh_instance_registry.ids[i] = INVALID_MESH_INSTANCE_ID;
    }
    memset(s_mesh_instance_registry.names, 0, sizeof(string_t*) * mesh_instance_name_registry_t::MAX_NUM_REGISTERED_MESH_INSTANCES);
}

void sm::mesh_instances_register_name(mesh_instance_id_t id, string_t* name)
{
    int empty_slot = -1;
    for(int i = 0; i < mesh_instance_name_registry_t::MAX_NUM_REGISTERED_MESH_INSTANCES; i++)
    {
        if(s_mesh_instance_registry.ids[i] == INVALID_MESH_INSTANCE_ID)
        {
            empty_slot = i;
            break;
        }
    }

    SM_ASSERT(empty_slot != -1);

    s_mesh_instance_registry.ids[empty_slot] = id;
    s_mesh_instance_registry.names[empty_slot] = name;
}

string_t* sm::mesh_instances_look_up_name(mesh_instance_id_t id)
{
    for(int i = 0; i < mesh_instance_name_registry_t::MAX_NUM_REGISTERED_MESH_INSTANCES; i++)
    {
        if(s_mesh_instance_registry.ids[i] == id)
        {
            return s_mesh_instance_registry.names[i];
        }
    }

    return nullptr;
}