// Version 3.0 by Chooka https://github.com/Chooka/obs-shaderfilter.git
// Version 2.0 by Exeldro https://github.com/exeldro/obs-shaderfilter
// Version 1.21 by Charles Fettinger https://github.com/Oncorporation/obs-shaderfilter
// original version by nleseul https://github.com/nleseul/obs-shaderfilter

#include "version.h"

#include "shader_filter_types.h"

#include "shader_filter.h"
#include "shader_filter_convert.h"
#include "shader_filter_main.h"
#include "shader_filter_params.h"
#include "shader_filter_props.h"
#include "shader_filter_render.h"
#include "shader_filter_transition.h"
#include "shader_filter_textures.h"
#include "shader_filter_templates.h"
#include "shader_filter_util.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("obs-shaderfilter-extended", "en-US")

float (*move_get_transition_filter)(obs_source_t *filter_from, obs_source_t **filter_to) = NULL;

struct obs_source_info shader_filter = {
	.id = "shader_filter_extended",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_SRGB | OBS_SOURCE_CUSTOM_DRAW,
	.create = shader_filter_create,
	.destroy = shader_filter_destroy,
	.update = shader_filter_update,
	.load = shader_filter_update,
	.video_tick = shader_filter_tick,
	.get_name = shader_filter_get_name,
	.get_defaults = shader_filter_defaults,
	.get_width = shader_filter_getwidth,
	.get_height = shader_filter_getheight,
	.video_render = shader_filter_render,
	.get_properties = shader_filter_properties,
	.video_get_color_space = shader_filter_get_color_space,
	.activate = shader_filter_activate,
	.deactivate = shader_filter_deactivate,
	.show = shader_filter_show,
	.hide = shader_filter_hide,
};

struct obs_source_info shader_transition = {
	.id = "shader_transition",
	.type = OBS_SOURCE_TYPE_TRANSITION,
	.create = shader_transition_create,
	.destroy = shader_filter_destroy,
	.update = shader_filter_update,
	.load = shader_filter_update,
	.video_tick = shader_filter_tick,
	.get_name = shader_transition_get_name,
	.audio_render = shader_transition_audio_render,
	.get_defaults = shader_transition_defaults,
	.video_render = shader_transition_video_render,
	.get_properties = shader_filter_properties,
	.video_get_color_space = shader_transition_get_color_space,
};

bool obs_module_load(void)
{
	blog(LOG_INFO, "[obs-shaderfilter-extended] loaded version %s", PROJECT_VERSION);
	obs_register_source(&shader_filter);
	obs_register_source(&shader_transition);
	return true;
}

void obs_module_unload(void) {}

void obs_module_post_load(void)
{
	if (obs_get_module("move-transition") == NULL)
		return;

	proc_handler_t *ph = obs_get_proc_handler();
	struct calldata cd;
	calldata_init(&cd);
	calldata_set_string(&cd, "filter_id", shader_filter.id);

	if (proc_handler_call(ph, "move_get_transition_filter_function", &cd)) {
		move_get_transition_filter = calldata_ptr(&cd, "callback");
	}

	calldata_free(&cd);
}
