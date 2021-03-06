#ifndef ENTITY_H
#define ENTITY_H
#include "transform.h"
#include "graphics/model.h"
#include "graphics/material.h"
#include "colliders.h"

typedef struct Entity Entity;

// Creates an entity and adds it to the scene
Entity* entity_create(const char* name, const char* material_name, const char* mesh_name, Transform transform);

const char* entity_get_name(Entity* entity);

Transform* entity_get_transform(Entity* entity);
Material* entity_get_material(Entity* entity);
Mesh* entity_get_mesh(Entity* entity);
const SphereCollider* entity_get_boundingsphere(Entity* entity);

// Is called once a frame
void entity_update(Entity* entity);

// Updates shader uniform for the entity from a mapped uniform buffer
void entity_update_shaderdata(Entity* entity, void* data_write, uint32_t index);

// Is only called irreguraly when command buffers are rebuilt
// Index refers to the index of the entity in the uniform  buffer
void entity_render(Entity* entity, CommandBuffer* commandbuffer, uint32_t index, VkDescriptorSet data_descriptors);

// Destroys and entity and removes it from the scene
void entity_destroy(Entity* entity);
#endif