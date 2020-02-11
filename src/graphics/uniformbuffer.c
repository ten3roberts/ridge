#include "head.h"
#include "log.h"
#include "graphics/vulkan_internal.h"
#include "texture.h"
#include "graphics/buffer.h"
#include <stdlib.h>
#include <stb_image.h>

VkDescriptorSetLayout layout = NULL;

VkDescriptorSetLayout* ub_get_layouts()
{
	return &layout;
}
uint32_t ub_get_layout_count()
{
	return 1;
}

typedef struct
{
	// Describes the currently used count of allocated descriptors
	uint32_t filled_count;
	// Describes the total count that was allocated
	uint32_t alloc_count;
	VkDescriptorPool pool;
} DescriptorPool;

DescriptorPool* descriptor_pools = NULL;
uint32_t descriptor_pool_count = 0;

DescriptorPool* sampler_descriptor_pools = NULL;
uint32_t sampler_descriptor_pool_count = 0;

static BufferPoolArray ub_pools = {VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 3 * 256 * 10, 0, NULL};

typedef struct
{
	Head head;
	// The size of one frame of the command buffer
	uint32_t size;
	uint32_t offsets[3];
	VkBuffer buffers[3];
	VkDeviceMemory memories[3];
	VkDescriptorSet descriptor_sets[3];
} UniformBuffer;

int ub_create_descriptor_set_layout()
{
	// Update the map that corresponds the binding with the layout
	VkDescriptorSetLayoutBinding uboLayoutBinding = {0};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;

	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	uboLayoutBinding.pImmutableSamplers = NULL; // Optional

	VkDescriptorSetLayoutBinding samplerLayoutBinding = {0};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = NULL;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding bindings[2] = {uboLayoutBinding, samplerLayoutBinding};

	VkDescriptorSetLayoutCreateInfo layoutInfo = {0};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = sizeof(bindings) / sizeof(*bindings);
	layoutInfo.pBindings = bindings;

	VkResult result = vkCreateDescriptorSetLayout(device, &layoutInfo, NULL, &layout);
	if (result != VK_SUCCESS)
	{
		LOG_E("Failed to create descriptor set layout - code %d", result);
		return -1;
	}
	return 0;
}

int ub_descriptor_pool_create()
{
	// Resize array
	LOG_S("Creating new descriptor pool");

	descriptor_pools = realloc(descriptor_pools, ++descriptor_pool_count * sizeof(DescriptorPool));
	DescriptorPool* descriptor_pool = &descriptor_pools[descriptor_pool_count - 1];

	descriptor_pool->filled_count = 0;
	descriptor_pool->alloc_count = swapchain_image_count * 100;

	VkDescriptorPoolSize poolSizes[2] = {0};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = descriptor_pool->alloc_count;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = descriptor_pool->alloc_count;

	VkDescriptorPoolCreateInfo poolInfo = {0};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = sizeof(poolSizes) / sizeof(*poolSizes);
	poolInfo.pPoolSizes = poolSizes;

	poolInfo.maxSets = descriptor_pool->alloc_count;

	VkResult result = vkCreateDescriptorPool(device, &poolInfo, NULL, &descriptor_pool->pool);
	if (result != VK_SUCCESS)
	{
		LOG_E("Failed to create descriptor pool - code %d", result);
		return -1;
	}
	return 0;
}

int ub_create_descriptor_sets(VkDescriptorSet* dst_descriptors, VkBuffer* buffers, uint32_t* offsets, uint32_t size,
							  VkImageView image_view, VkSampler sampler)
{
	VkDescriptorSetLayout* layouts = malloc(swapchain_image_count * sizeof(VkDescriptorSetLayout));

	// Fill the layouts for all swapchain image count
	for (size_t i = 0; i < swapchain_image_count; i++)
		layouts[i] = layout;

	VkDescriptorSetAllocateInfo allocInfo = {0};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	if (descriptor_pool_count == 0)
	{
		ub_descriptor_pool_create(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	}

	// Iterate and find a pool that is not full
	for (uint32_t i = 0; i < descriptor_pool_count; i++)
	{
		if (descriptor_pools[i].filled_count + 3 >= descriptor_pools[i].alloc_count)
		{
			// At last pool
			if (i == descriptor_pool_count - 1)
			{
				ub_descriptor_pool_create();
			}
			continue;
		}
		// Found a good pool
		allocInfo.descriptorPool = descriptor_pools[i].pool;
		descriptor_pools[i].filled_count += 3;
	}

	allocInfo.descriptorSetCount = swapchain_image_count;
	allocInfo.pSetLayouts = layouts;

	vkAllocateDescriptorSets(device, &allocInfo, dst_descriptors);
	for (size_t i = 0; i < swapchain_image_count; i++)
	{
		VkDescriptorBufferInfo bufferInfo = {0};
		bufferInfo.buffer = buffers[i];
		bufferInfo.offset = offsets[i];
		bufferInfo.range = size;

		VkDescriptorImageInfo imageInfo = {0};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = texture_get_image_view(tex);
		imageInfo.sampler = texture_get_sampler(tex);

		VkWriteDescriptorSet descriptorWrites[2] = {0};
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = dst_descriptors[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;
		descriptorWrites[0].pImageInfo = NULL;
		descriptorWrites[0].pTexelBufferView = NULL;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = dst_descriptors[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pBufferInfo = NULL;
		descriptorWrites[1].pImageInfo = &imageInfo;
		descriptorWrites[1].pTexelBufferView = NULL;

		vkUpdateDescriptorSets(device, 2, descriptorWrites, 0,
							   NULL);
	}

	free(layouts);
	return 0;
}

UniformBuffer* ub_create(uint32_t size, uint32_t binding)
{
	LOG_S("Creating uniform buffer");
	UniformBuffer* ub = malloc(sizeof(UniformBuffer));
	ub->head.type = RT_UNIFORMBUFFER;
	ub->head.id = 0;
	ub->size = size;

	// Find a free pool
	for (int i = 0; i < swapchain_image_count; i++)
	{
		buffer_pool_array_get(&ub_pools, size, &ub->buffers[i], &ub->memories[i], &ub->offsets[i]);
	}

	// Create descriptor sets
	ub_create_descriptor_sets(ub->descriptor_sets, ub->buffers, ub->offsets, ub->size, NULL, NULL);
	return ub;
}

void ub_update(UniformBuffer* ub, void* data, uint32_t i)
{
	void* data_map = NULL;
	vkMapMemory(device, ub->memories[i], ub->offsets[i], ub->size, 0, &data_map);
	if (data_map == NULL)
	{
		LOG_E("Failed to map memory for uniform buffer");
		return;
	}
	memcpy(data_map, data, ub->size);
	vkUnmapMemory(device, ub->memories[i]);
}

void ub_destroy(UniformBuffer* ub)
{
	LOG_S("Destroying uniform buffer");
	for (size_t i = 0; i < swapchain_image_count; i++)
	{
		// vkDestroyBuffer(device, ub->buffers[i], NULL);
		// vkFreeMemory(device, ub->memories[i], NULL);
	}
	free(ub);
}

void ub_bind(UniformBuffer* ub, VkCommandBuffer command_buffer, int i)
{
	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1,
							&ub->descriptor_sets[i], 0, NULL);
}

void ub_pools_destroy()
{
	buffer_pool_array_destroy(&ub_pools);
	for (uint32_t i = 0; i < descriptor_pool_count; i++)
	{
		vkDestroyDescriptorPool(device, descriptor_pools[i].pool, NULL);
	}

	free(descriptor_pools);
	descriptor_pool_count = 0;

	/*for (uint32_t i = 0; i < descriptor_layout_count; i++)
	{
		vkDestroyDescriptorSetLayout(device, descriptor_layouts[i], NULL);
	}
	free(descriptor_layouts);
	free(layout_binding_map);
	descriptor_layout_count = 0;*/
}
