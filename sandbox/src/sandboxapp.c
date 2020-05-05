#include "ridge.h"

static Window* window = NULL;

int swapchain_resize = 0;

int application_start(int argc, char** argv)
{
	Timer timer = timer_start(CT_WALL_TICKS);

	time_init();

	settings_load();

	window = window_create("sandbox", settings_get_resolution().x, settings_get_resolution().y, settings_get_window_style(), 1);
	window_set_icon(window, "./assets/textures/ridge64.png", "./assets/textures/ridge1024.png");

	input_init(window);
	graphics_init(window, GLOBAL_LAYOUT_DEFAULT);
	renderer_init();

	LOG_S("Initialization took %f ms", timer_stop(&timer) * 1000);

	timer_reset(&timer);
	time_init();

	Scene* scene = scene_create("main");
	(void)model_load_collada("./assets/models/cube.dae");
	(void)material_load("./assets/materials/concrete.json");
	(void)material_load("./assets/materials/grid.json");

	Camera* camera = camera_create_perspective("main", (Transform){(vec3){5, 0, 0}}, window_get_aspect(window), 1, 0.1, 100);

	Entity* entity1 = entity_create("entity1", "grid", "Cube", (Transform){(vec3){0, 0, -1}, quat_identity, vec3_one});
	Entity* entity2 = entity_create("entity2", "concrete", "Cube", (Transform){(vec3){5, 0.5, 10}, quat_identity, vec3_one});

	while (!window_get_close(window))
	{
		// Poll window events
		window_update(window);
		renderer_begin();

		scene_update(scene);

		entity_get_transform(entity1)->position.z = -5;
		entity_get_transform(entity1)->rotation	  = quat_euler((vec3){0, time_elapsed(), 0});
		entity_get_transform(entity2)->rotation	  = quat_euler((vec3){time_elapsed(), 0, 0});

		// Spin camera
		camera_get_transform(camera)->rotation = quat_euler((vec3){0, time_elapsed(), 0});
		graphics_update_scene_data();
		input_update();

		time_update();
		renderer_submit();

		if (timer_duration(&timer) > 2.0f)
		{
			timer_reset(&timer);
			LOG("Framerate %d %f", time_framecount(), time_framerate());
		}
	}
	scene_destroy_entities(scene);
	scene_destroy(scene);
	graphics_terminate();
	LOG_S("Terminating");
	window_destroy(window);
	settings_save();

	return 0;
}

void application_send_event(Event event)
{
	// Recreate swapchain on resize
	if (event.type == EVENT_WINDOW_RESIZE && event.idata[0] != 0 && event.idata[1] != 0)
	{
		LOG("%d %d", event.idata[0], event.idata[1]);
		renderer_resize();
	}
	if (event.type == EVENT_KEY)
		LOG("Key pressed  : %d, %c", event.idata[0], event.idata[0]);
	if (event.type == EVENT_KEY || event.type == EVENT_MOUSE_MOVED || event.type == EVENT_MOUSE_SCROLLED)
		input_send_event(&event);
}

void* application_get_window()
{
	return window;
}
