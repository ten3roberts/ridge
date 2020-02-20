#include "log.h"
#include "vulkan_members.h"
#include "swapchain.h"
#include "cr_time.h"
#include "graphics/uniformbuffer.h"
#include "math/quaternion.h"

void renderer_draw()
{
	// Skip rendering if window is minimized
	if (window_get_minimized(window))
	{
		return;
	}
	vkWaitForFences(device, 1, &in_flight_fences[current_frame], VK_TRUE, UINT64_MAX);

	uint32_t image_index;
	vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, semaphores_image_available[current_frame], VK_NULL_HANDLE,
						  &image_index);

	// Update uniform buffer
	TransformType transform_buffer;
	quaternion rotation = quat_axis_angle((vec3){0, 0.1, 1}, time_elapsed());

	mat4 rot = quat_to_mat4(rotation);
	mat4 pos = mat4_translate((vec3){time_elapsed()*-0.5, sinf(time_elapsed()) * 0.5, -time_elapsed() + -2});
	mat4 scale = mat4_scale((vec3){1, 1, 1});
	transform_buffer.model = mat4_mul(&rot, &pos);

	transform_buffer.view = mat4_identity;
	// transform_buffer.proj = mat4_perspective(window_get_width(window) / window_get_height(window), 1, 0, 10);
	transform_buffer.proj = mat4_ortho(window_get_aspect(window), 1, 0, 10);
	ub_update(ub, &transform_buffer, image_index);

	// Second
	rotation = quat_axis_angle((vec3){1, 1, 0}, time_elapsed());
	rot = quat_to_mat4(rotation);

	pos = mat4_translate((vec3){time_elapsed()*0.1, -sinf(time_elapsed()) * 0.5, -time_elapsed()*0.5});
	scale = mat4_scale((vec3){0.5, 0.5, 0.5});
	transform_buffer.model = mat4_mul(&rot, &pos);
	transform_buffer.model = mat4_mul(&transform_buffer.model, &scale);

	ub_update(ub2, &transform_buffer, image_index);

	// Check if a previous frame is using this image (i.e. there is its fence to wait on)
	if (images_in_flight[image_index] != VK_NULL_HANDLE)
	{
		vkWaitForFences(device, 1, &images_in_flight[image_index], VK_TRUE, UINT64_MAX);
	}

	// Mark the image as now being in use by this frame
	images_in_flight[image_index] = in_flight_fences[current_frame];

	// Submit render queue
	// Specifies which semaphores to wait for before execution
	// Specify to wait for image available before writing to swapchain
	VkSubmitInfo submit_info = {0};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	VkSemaphore wait_semaphores[] = {semaphores_image_available[current_frame]};
	VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submit_info.waitSemaphoreCount = sizeof wait_semaphores / sizeof *wait_semaphores;
	submit_info.pWaitSemaphores = wait_semaphores;
	submit_info.pWaitDstStageMask = wait_stages;

	// Specify which command buffers to submit for execution
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &command_buffers[image_index];

	// Specify which semaphores to signal on completion
	VkSemaphore signal_semaphores[] = {semaphores_render_finished[current_frame]};
	submit_info.signalSemaphoreCount = sizeof signal_semaphores / sizeof *signal_semaphores;
	submit_info.pSignalSemaphores = signal_semaphores;

	// Synchronise CPU-GPU
	vkResetFences(device, 1, &in_flight_fences[current_frame]);

	VkResult result = vkQueueSubmit(graphics_queue, 1, &submit_info, in_flight_fences[current_frame]);
	if (result != VK_SUCCESS)
	{
		LOG_E("Failed to submit draw command buffer - code %d", result);
		return;
	}

	// Presentation
	VkPresentInfoKHR present_info = {0};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.waitSemaphoreCount = sizeof signal_semaphores / sizeof *signal_semaphores;
	present_info.pWaitSemaphores = signal_semaphores;

	VkSwapchainKHR swapchains[] = {swapchain};
	present_info.swapchainCount = sizeof swapchains / sizeof *swapchains;
	present_info.pSwapchains = swapchains;
	present_info.pImageIndices = &image_index;

	present_info.pResults = NULL; // Optional

	result = vkQueuePresentKHR(present_queue, &present_info);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		swapchain_recreate();
		return;
	}
	if (result != VK_SUCCESS)
	{
		LOG_E("Failed to present swapchain image");
		return;
	}

	current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}