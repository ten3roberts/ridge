#pragma once
#include "math/mat4.h"
#include <vulkan/vulkan.h>
#include "graphics/texture.h"
#include <stdint.h>

#define CS_WHOLE_SIZE -1

typedef struct
{
	mat4 model;
	mat4 view;
	mat4 proj;
} TransformType;

typedef struct UniformBuffer UniformBuffer;

// Creates a descriptor set layout from the specified bindings
// Used when creating descriptors and during pipeline creation
int descriptorlayout_create(VkDescriptorSetLayoutBinding* bindings, uint32_t binding_count,
							VkDescriptorSetLayout* dst_layout);

// Creates multiple descriptors, one for each frame in flight (swapchain_image_count)
// Writes the buffers and samplers to each frame's descriptor as specified in bindings
// The number of uniformbuffers should match the bindings
// The number of textures should match the bindings
// dst_descriptors should be an array of swapchain_image_count length. Arrays data will be overwritten
int descriptorset_create(VkDescriptorSetLayout layout, VkDescriptorSetLayoutBinding* bindings, uint32_t binding_count,
						 UniformBuffer** uniformbuffers, Texture** textures, VkDescriptorSet* dst_descriptors);

// Creates and allocates memory for a uniform buffer
// Internally holds one buffer per frame in flight to avoid simultaneous read and writes
// Uniform buffer is completely agnostic to the shader layout and binding
// To bind a uniform buffer you need to create a descriptor layout and set
UniformBuffer* ub_create(uint32_t size, uint32_t binding);

// Updates a uniform buffer
// Maps memory from the GPU to the CPU
// If size is -1 or CS_WHOLE_SIZE, the rest of the buffer will be written after offset
// Writes size amount of bytes from data after offset
// The frame specifies which of the internal buffers to map
// If frame is -1, the current frame to render will be used (result of renderer_get_frame)
// NOTE: offset + size should be less than or equal to size of the uniform buffer
void ub_update(UniformBuffer* ub, void* data, uint32_t offset, uint32_t size, uint32_t frame);
void ub_destroy(UniformBuffer* ub);

// Destroys all UniformBuffer pools in the end of the programs
// The pool were first created implicitly when a UniformBuffer was created
void ub_pools_destroy();