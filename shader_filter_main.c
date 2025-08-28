// Version 3.0 by Chooka https://github.com/Chooka/obs-shaderfilter.git
// Version 2.0 by Exeldro https://github.com/exeldro/obs-shaderfilter
// Version 1.21 by Charles Fettinger https://github.com/Oncorporation/obs-shaderfilter
// original version by nleseul https://github.com/nleseul/obs-shaderfilter

#include "shader_filter_types.h"
#include "shader_filter_main.h"
#include "shader_filter_params.h"
#include "shader_filter_util.h"
#include "shader_filter_templates.h"

#include <util/platform.h>


void *shader_filter_create(obs_data_t *settings, obs_source_t *source)
{
	struct shader_filter_data *filter = bzalloc(sizeof(struct shader_filter_data));
	filter->context = source;
	filter->reload_effect = true;

	dstr_init(&filter->last_path);
	dstr_copy(&filter->last_path, obs_data_get_string(settings, "shader_file_name"));
	filter->last_from_file = obs_data_get_bool(settings, "from_file");
	filter->rand_instance_f = (float)((double)rand_interval(0, 10000) / (double)10000);
	filter->rand_activation_f = (float)((double)rand_interval(0, 10000) / (double)10000);

	da_init(filter->stored_param_list);
	load_output_effect(filter);
	obs_source_update(source, settings);

	return filter;
}

void shader_filter_destroy(void *data)
{
	struct shader_filter_data *filter = data;
	shader_filter_clear_params(filter);

	obs_enter_graphics();
	if (filter->effect)
		gs_effect_destroy(filter->effect);
	if (filter->output_effect)
		gs_effect_destroy(filter->output_effect);
	if (filter->input_texrender)
		gs_texrender_destroy(filter->input_texrender);
	if (filter->output_texrender)
		gs_texrender_destroy(filter->output_texrender);
	if (filter->previous_input_texrender)
		gs_texrender_destroy(filter->previous_input_texrender);
	if (filter->previous_output_texrender)
		gs_texrender_destroy(filter->previous_output_texrender);
	if (filter->sprite_buffer)
		gs_vertexbuffer_destroy(filter->sprite_buffer);
	obs_leave_graphics();

	dstr_free(&filter->last_path);
	da_free(filter->stored_param_list);

	bfree(filter);
}

void shader_filter_update(void *data, obs_data_t *settings)
{
	struct shader_filter_data *filter = data;

	// Get expansions. Will be used in the video_tick() callback.

	filter->expand_left = (int)obs_data_get_int(settings, "expand_left");
	filter->expand_right = (int)obs_data_get_int(settings, "expand_right");
	filter->expand_top = (int)obs_data_get_int(settings, "expand_top");
	filter->expand_bottom = (int)obs_data_get_int(settings, "expand_bottom");
	filter->rand_activation_f = (float)((double)rand_interval(0, 10000) / (double)10000);

	if (filter->reload_effect) {
		filter->reload_effect = false;
		shader_filter_reload_effect(filter);
		obs_source_update_properties(filter->context);
	}

	size_t param_count = filter->stored_param_list.num;
	for (size_t param_index = 0; param_index < param_count; param_index++) {
		struct effect_param_data *param = (filter->stored_param_list.array + param_index);
		//gs_eparam_t *annot = gs_param_get_annotation_by_idx(param->param, param_index);
		const char *param_name = param->name.array;
		struct dstr sources_name = {0};
		obs_source_t *source = NULL;
		void *default_value = gs_effect_get_default_val(param->param);
		param->has_default = false;
		switch (param->type) {
		case GS_SHADER_PARAM_BOOL:
			if (default_value != NULL) {
				obs_data_set_default_bool(settings, param_name, *(bool *)default_value);
				param->default_value.i = *(bool *)default_value;
				param->has_default = true;
			}
			param->value.i = obs_data_get_bool(settings, param_name);
			break;
		case GS_SHADER_PARAM_FLOAT:
			if (default_value != NULL) {
				obs_data_set_default_double(settings, param_name, *(float *)default_value);
				param->default_value.f = *(float *)default_value;
				param->has_default = true;
			}
			param->value.f = obs_data_get_double(settings, param_name);
			break;
		case GS_SHADER_PARAM_INT:
			if (default_value != NULL) {
				obs_data_set_default_int(settings, param_name, *(int *)default_value);
				param->default_value.i = *(int *)default_value;
				param->has_default = true;
			}
			param->value.i = obs_data_get_int(settings, param_name);
			break;
		case GS_SHADER_PARAM_VEC2: {
			struct vec2 *xy = default_value;

			for (size_t i = 0; i < 2; i++) {
				dstr_printf(&sources_name, "%s_%zu", param_name, i);
				if (xy != NULL) {
					obs_data_set_default_double(settings, sources_name.array, xy->ptr[i]);
					param->default_value.vec2.ptr[i] = xy->ptr[i];
					param->has_default = true;
				}
				param->value.vec2.ptr[i] = (float)obs_data_get_double(settings, sources_name.array);
			}
			dstr_free(&sources_name);
			break;
		}
		case GS_SHADER_PARAM_VEC3: {
			struct vec3 *rgb = default_value;
			if (param->widget_type.array && strcmp(param->widget_type.array, "slider") == 0) {
				for (size_t i = 0; i < 3; i++) {
					dstr_printf(&sources_name, "%s_%zu", param_name, i);
					if (rgb != NULL) {
						obs_data_set_default_double(settings, sources_name.array, rgb->ptr[i]);
						param->default_value.vec3.ptr[i] = rgb->ptr[i];
						param->has_default = true;
					}
					param->value.vec3.ptr[i] = (float)obs_data_get_double(settings, sources_name.array);
				}
				dstr_free(&sources_name);
			} else {
				if (rgb != NULL) {
					struct vec4 rgba;
					vec4_from_vec3(&rgba, rgb);
					obs_data_set_default_int(settings, param_name, vec4_to_rgba(&rgba));
					param->default_value.vec4 = rgba;
					param->has_default = true;
				} else {
					// Hack to ensure we have a default...(white)
					obs_data_set_default_int(settings, param_name, 0xffffffff);
				}
				vec4_from_rgba(&param->value.vec4, (uint32_t)obs_data_get_int(settings, param_name));
			}
			break;
		}
		case GS_SHADER_PARAM_VEC4: {
			struct vec4 *rgba = default_value;
			if (param->widget_type.array && strcmp(param->widget_type.array, "slider") == 0) {
				for (size_t i = 0; i < 4; i++) {
					dstr_printf(&sources_name, "%s_%zu", param_name, i);
					if (rgba != NULL) {
						obs_data_set_default_double(settings, sources_name.array, rgba->ptr[i]);
						param->default_value.vec4.ptr[i] = rgba->ptr[i];
						param->has_default = true;
					}
					param->value.vec4.ptr[i] = (float)obs_data_get_double(settings, sources_name.array);
				}
				dstr_free(&sources_name);
			} else {
				if (rgba != NULL) {
					obs_data_set_default_int(settings, param_name, vec4_to_rgba(rgba));
					param->default_value.vec4 = *rgba;
					param->has_default = true;
				} else {
					// Hack to ensure we have a default...(white)
					obs_data_set_default_int(settings, param_name, 0xffffffff);
				}
				vec4_from_rgba(&param->value.vec4, (uint32_t)obs_data_get_int(settings, param_name));
			}
			break;
		}
		case GS_SHADER_PARAM_TEXTURE:
			dstr_init_copy_dstr(&sources_name, &param->name);
			dstr_cat(&sources_name, "_source");
			const char *sn = obs_data_get_string(settings, sources_name.array);
			dstr_free(&sources_name);
			source = obs_weak_source_get_source(param->source);
			if (source && strcmp(obs_source_get_name(source), sn) != 0) {
				obs_source_release(source);
				source = NULL;
			}
			if (!source)
				source = (sn && strlen(sn)) ? obs_get_source_by_name(sn) : NULL;
			if (source) {
				if (!obs_weak_source_references_source(param->source, source)) {
					if ((!filter->transition || filter->prev_transitioning) &&
					    obs_source_active(filter->context))
						obs_source_inc_active(source);
					if ((!filter->transition || filter->prev_transitioning) &&
					    obs_source_showing(filter->context))
						obs_source_inc_showing(source);

					obs_source_t *old_source = obs_weak_source_get_source(param->source);
					if (old_source) {
						if ((!filter->transition || filter->prev_transitioning) &&
						    obs_source_active(filter->context))
							obs_source_dec_active(old_source);
						if ((!filter->transition || filter->prev_transitioning) &&
						    obs_source_showing(filter->context))
							obs_source_dec_showing(old_source);
						obs_source_release(old_source);
					}
					obs_weak_source_release(param->source);
					param->source = obs_source_get_weak_source(source);
				}
				obs_source_release(source);
				if (param->image) {
					gs_image_file_free(param->image);
					param->image = NULL;
				}
				dstr_free(&param->path);
			} else {
				const char *path = default_value;
				if (!obs_data_has_user_value(settings, param_name) && path && strlen(path)) {
					if (os_file_exists(path)) {
						char *abs_path = os_get_abs_path_ptr(path);
						obs_data_set_default_string(settings, param_name, abs_path);
						bfree(abs_path);
						param->has_default = true;
					} else {
						struct dstr texture_path = {0};
						dstr_init(&texture_path);
						dstr_cat(&texture_path, obs_get_module_data_path(obs_current_module()));
						dstr_cat(&texture_path, "/textures/");
						dstr_cat(&texture_path, path);
						char *abs_path = os_get_abs_path_ptr(texture_path.array);
						if (os_file_exists(abs_path)) {
							obs_data_set_default_string(settings, param_name, abs_path);
							param->has_default = true;
						}
						bfree(abs_path);
						dstr_free(&texture_path);
					}
				}
				path = obs_data_get_string(settings, param_name);
				bool n = false;
				if (param->image == NULL) {
					param->image = bzalloc(sizeof(gs_image_file_t));
					n = true;
				}
				if (n || !path || !param->path.array || strcmp(path, param->path.array) != 0) {

					if (!n) {
						obs_enter_graphics();
						gs_image_file_free(param->image);
						obs_leave_graphics();
					}
					gs_image_file_init(param->image, path);
					dstr_copy(&param->path, path);
					obs_enter_graphics();
					gs_image_file_init_texture(param->image);
					obs_leave_graphics();
				}
				obs_source_t *old_source = obs_weak_source_get_source(param->source);
				if (old_source) {
					if ((!filter->transition || filter->prev_transitioning) &&
					    obs_source_active(filter->context))
						obs_source_dec_active(old_source);
					if ((!filter->transition || filter->prev_transitioning) &&
					    obs_source_showing(filter->context))
						obs_source_dec_showing(old_source);
					obs_source_release(old_source);
				}
				obs_weak_source_release(param->source);
				param->source = NULL;
			}
			break;
		case GS_SHADER_PARAM_STRING:
			if (default_value != NULL) {
				obs_data_set_default_string(settings, param_name, (const char *)default_value);
				param->has_default = true;
			}
			param->value.string = (char *)obs_data_get_string(settings, param_name);
			break;
		default:;
		}
		bfree(default_value);
	}
}

void shader_filter_tick(void *data, float seconds)
{
	struct shader_filter_data *filter = data;
	obs_source_t *target = filter->transition ? filter->context : obs_filter_get_target(filter->context);
	if (!target)
		return;

	int base_width = obs_source_get_base_width(target);
	int base_height = obs_source_get_base_height(target);

	filter->total_width = filter->expand_left + base_width + filter->expand_right;
	filter->total_height = filter->expand_top + base_height + filter->expand_bottom;

	filter->uv_size.x = (float)filter->total_width;
	filter->uv_size.y = (float)filter->total_height;

	filter->uv_scale.x = (float)filter->total_width / base_width;
	filter->uv_scale.y = (float)filter->total_height / base_height;

	filter->uv_offset.x = (float)(-filter->expand_left) / base_width;
	filter->uv_offset.y = (float)(-filter->expand_top) / base_height;

	filter->uv_pixel_interval.x = 1.0f / base_width;
	filter->uv_pixel_interval.y = 1.0f / base_height;

	if (filter->shader_start_time == 0.0f)
		filter->shader_start_time = filter->elapsed_time + seconds;

	filter->elapsed_time += seconds;
	filter->elapsed_time_loop += seconds;
	if (filter->elapsed_time_loop > 1.0f) {
		filter->elapsed_time_loop -= 1.0f;
		filter->loops += 1;
		if (filter->loops >= 4194304)
			filter->loops = -filter->loops;
	}

	filter->local_time = (float)(os_gettime_ns() / 1000000000.0);

	if (filter->enabled != obs_source_enabled(filter->context)) {
		filter->enabled = !filter->enabled;
		if (filter->enabled)
			filter->shader_enable_time = filter->elapsed_time;
	}

	obs_source_t *parent = obs_filter_get_parent(filter->context);
	if (obs_source_enabled(filter->context) && parent && obs_source_active(parent))
		filter->shader_active_time += seconds;
	else
		filter->shader_active_time = 0.0f;

	if (obs_source_enabled(filter->context) && parent && obs_source_showing(parent))
		filter->shader_show_time += seconds;
	else
		filter->shader_show_time = 0.0f;

	filter->rand_f = (float)((double)rand_interval(0, 10000) / (double)10000);
	filter->output_rendered = false;
	filter->input_rendered = false;
}

void shader_filter_activate(void *data)
{
	shader_filter_param_source_action(data, obs_source_inc_active);
}

void shader_filter_deactivate(void *data)
{
	shader_filter_param_source_action(data, obs_source_dec_active);
}

void shader_filter_show(void *data)
{
	shader_filter_param_source_action(data, obs_source_inc_showing);
}

void shader_filter_hide(void *data)
{
	shader_filter_param_source_action(data, obs_source_dec_showing);
}

const char *shader_filter_get_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("Shader Filter Extended");
}

void shader_filter_defaults(obs_data_t *settings)
{
	obs_data_set_default_string(settings, "shader_text", effect_template_default_image_shader);
}

uint32_t shader_filter_getwidth(void *data)
{
	struct shader_filter_data *filter = data;

	return filter->total_width;
}

uint32_t shader_filter_getheight(void *data)
{
	struct shader_filter_data *filter = data;

	return filter->total_height;
}

enum gs_color_space shader_filter_get_color_space(void *data, size_t count, const enum gs_color_space *preferred_spaces)
{
	UNUSED_PARAMETER(count);
	UNUSED_PARAMETER(preferred_spaces);
	struct shader_filter_data *filter = data;
	obs_source_t *target = obs_filter_get_target(filter->context);
	const enum gs_color_space potential_spaces[] = {
		GS_CS_SRGB,
		GS_CS_SRGB_16F,
		GS_CS_709_EXTENDED,
	};
	return obs_source_get_color_space(target, OBS_COUNTOF(potential_spaces), potential_spaces);
}

void load_output_effect(struct shader_filter_data *filter)
{
	if (filter->output_effect != NULL) {
		obs_enter_graphics();
		gs_effect_destroy(filter->output_effect);
		filter->output_effect = NULL;
		obs_leave_graphics();
	}

	char *shader_text = NULL;
	struct dstr filename = {0};
	dstr_cat(&filename, obs_get_module_data_path(obs_current_module()));
	dstr_cat(&filename, "/internal/render_output.effect");
	char *abs_path = os_get_abs_path_ptr(filename.array);
	if (abs_path) {
		shader_text = load_shader_from_file(abs_path);
		bfree(abs_path);
	}
	if (!shader_text)
		shader_text = load_shader_from_file(filename.array);

	char *errors = NULL;
	dstr_free(&filename);

	obs_enter_graphics();
	filter->output_effect = gs_effect_create(shader_text, NULL, &errors);
	obs_leave_graphics();

	bfree(shader_text);
	if (filter->output_effect == NULL) {
		blog(LOG_WARNING, "[obs-shaderfilter-extended] Unable to load render_output.effect file.  Errors:\n%s",
		     (errors == NULL || strlen(errors) == 0 ? "(None)" : errors));
		bfree(errors);
	} else {
		size_t effect_count = gs_effect_get_num_params(filter->output_effect);
		for (size_t effect_index = 0; effect_index < effect_count; effect_index++) {
			gs_eparam_t *param = gs_effect_get_param_by_idx(filter->output_effect, effect_index);
			struct gs_effect_param_info info;
			gs_effect_get_param_info(param, &info);
			if (strcmp(info.name, "output_image") == 0) {
				filter->param_output_image = param;
			}
		}
	}
}

char *load_shader_from_file(const char *file_name)
{
	char *file_ptr = os_quick_read_utf8_file(file_name);
	if (file_ptr == NULL) {
		blog(LOG_WARNING, "[obs-shaderfilter-extended] failed to read file: %s", file_name);
		return NULL;
	}

	char **lines = strlist_split(file_ptr, '\n', true);
	struct dstr shader_file;
	dstr_init(&shader_file);

	size_t line_i = 0;
	while (lines[line_i] != NULL) {
		char *line = lines[line_i];
		line_i++;
		if (strncmp(line, "#include", 8) == 0) {
			char *pos = strrchr(file_name, '/');
			const size_t length = pos - file_name + 1;
			struct dstr include_path = {0};
			dstr_ncopy(&include_path, file_name, length);
			char *start = strchr(line, '"') + 1;
			char *end = strrchr(line, '"');
			dstr_ncat(&include_path, start, end - start);
			char *abs_include_path = os_get_abs_path_ptr(include_path.array);
			char *file_contents = load_shader_from_file(abs_include_path);
			dstr_cat(&shader_file, file_contents);
			dstr_cat(&shader_file, "\n");
			bfree(abs_include_path);
			bfree(file_contents);
			dstr_free(&include_path);
		} else {
			dstr_cat(&shader_file, line);
			dstr_cat(&shader_file, "\n");
		}
	}

	bfree(file_ptr);
	strlist_free(lines);
	return shader_file.array;
}

void shader_filter_reload_effect(struct shader_filter_data *filter)
{
	obs_data_t *settings = obs_source_get_settings(filter->context);

	// First, clean up the old effect and all references to it.
	filter->shader_start_time = 0.0f;
	shader_filter_clear_params(filter);

	if (filter->effect != NULL) {
		obs_enter_graphics();
		gs_effect_destroy(filter->effect);
		filter->effect = NULL;
		obs_leave_graphics();
	}

	// Load text and build the effect from the template, if necessary.
	char *shader_text = NULL;
	bool use_template = !obs_data_get_bool(settings, "override_entire_effect");

	if (obs_data_get_bool(settings, "from_file")) {
		const char *file_name = obs_data_get_string(settings, "shader_file_name");
		if (!strlen(file_name)) {
			obs_data_unset_user_value(settings, "last_error");
			goto end;
		}
		shader_text = load_shader_from_file(file_name);
		if (!shader_text) {
			obs_data_set_string(settings, "last_error", obs_module_text("ShaderFilterExtended.FileLoadFailed"));
			goto end;
		}
	} else {
		shader_text = bstrdup(obs_data_get_string(settings, "shader_text"));
		use_template = true;
	}
	filter->use_template = use_template;

	struct dstr effect_text = {0};

	if (use_template) {
		dstr_cat(&effect_text, effect_template_begin);
	}

	if (shader_text) {
		dstr_cat(&effect_text, shader_text);
		bfree(shader_text);
	}

	if (use_template) {
		dstr_cat(&effect_text, effect_template_end);
	}

	// Create the effect.
	char *errors = NULL;

	obs_enter_graphics();
	int device_type = gs_get_device_type();
	if (device_type == GS_DEVICE_OPENGL) {
		dstr_replace(&effect_text, "[loop]", "");
		dstr_insert(&effect_text, 0, "#define OPENGL 1\n");
	}

	if (effect_text.len && dstr_find(&effect_text, "#define USE_PM_ALPHA 1")) {
		filter->use_pm_alpha = true;
	} else {
		filter->use_pm_alpha = false;
	}

	if (filter->effect)
		gs_effect_destroy(filter->effect);
	filter->effect = gs_effect_create(effect_text.array, NULL, &errors);
	obs_leave_graphics();

	if (filter->effect == NULL) {
		blog(LOG_WARNING, "[obs-shaderfilter-extended] Unable to create effect. Errors returned from parser:\n%s",
		     (errors == NULL || strlen(errors) == 0 ? "(None)" : errors));
		if (errors && strlen(errors)) {
			obs_data_set_string(settings, "last_error", errors);
		} else {
			obs_data_set_string(settings, "last_error", obs_module_text("ShaderFilterExtended.Unknown"));
		}
		dstr_free(&effect_text);
		bfree(errors);
		goto end;
	} else {
		dstr_free(&effect_text);
		obs_data_unset_user_value(settings, "last_error");
	}

	// Store references to the new effect's parameters.
	da_free(filter->stored_param_list);

	size_t effect_count = gs_effect_get_num_params(filter->effect);
	for (size_t effect_index = 0; effect_index < effect_count; effect_index++) {
		gs_eparam_t *param = gs_effect_get_param_by_idx(filter->effect, effect_index);
		if (!param)
			continue;
		struct gs_effect_param_info info;
		gs_effect_get_param_info(param, &info);

		if (strcmp(info.name, "uv_offset") == 0) {
			filter->param_uv_offset = param;
		} else if (strcmp(info.name, "uv_scale") == 0) {
			filter->param_uv_scale = param;
		} else if (strcmp(info.name, "uv_pixel_interval") == 0) {
			filter->param_uv_pixel_interval = param;
		} else if (strcmp(info.name, "uv_size") == 0) {
			filter->param_uv_size = param;
		} else if (strcmp(info.name, "current_time_ms") == 0) {
			filter->param_current_time_ms = param;
		} else if (strcmp(info.name, "current_time_sec") == 0) {
			filter->param_current_time_sec = param;
		} else if (strcmp(info.name, "current_time_min") == 0) {
			filter->param_current_time_min = param;
		} else if (strcmp(info.name, "current_time_hour") == 0) {
			filter->param_current_time_hour = param;
		} else if (strcmp(info.name, "current_time_day_of_week") == 0) {
			filter->param_current_time_day_of_week = param;
		} else if (strcmp(info.name, "current_time_day_of_month") == 0) {
			filter->param_current_time_day_of_month = param;
		} else if (strcmp(info.name, "current_time_month") == 0) {
			filter->param_current_time_month = param;
		} else if (strcmp(info.name, "current_time_day_of_year") == 0) {
			filter->param_current_time_day_of_year = param;
		} else if (strcmp(info.name, "current_time_year") == 0) {
			filter->param_current_time_year = param;
		} else if (strcmp(info.name, "elapsed_time") == 0) {
			filter->param_elapsed_time = param;
		} else if (strcmp(info.name, "elapsed_time_start") == 0) {
			filter->param_elapsed_time_start = param;
		} else if (strcmp(info.name, "elapsed_time_show") == 0) {
			filter->param_elapsed_time_show = param;
		} else if (strcmp(info.name, "elapsed_time_active") == 0) {
			filter->param_elapsed_time_active = param;
		} else if (strcmp(info.name, "elapsed_time_enable") == 0) {
			filter->param_elapsed_time_enable = param;
		} else if (strcmp(info.name, "rand_f") == 0) {
			filter->param_rand_f = param;
		} else if (strcmp(info.name, "rand_activation_f") == 0) {
			filter->param_rand_activation_f = param;
		} else if (strcmp(info.name, "rand_instance_f") == 0) {
			filter->param_rand_instance_f = param;
		} else if (strcmp(info.name, "loops") == 0) {
			filter->param_loops = param;
		} else if (strcmp(info.name, "loop_second") == 0) {
			filter->param_loop_second = param;
		} else if (strcmp(info.name, "local_time") == 0) {
			filter->param_local_time = param;
		} else if (strcmp(info.name, "ViewProj") == 0) {
			// Nothing.
		} else if (strcmp(info.name, "image") == 0) {
			filter->param_image = param;
		} else if (strcmp(info.name, "previous_image") == 0) {
			filter->param_previous_image = param;
		} else if (strcmp(info.name, "previous_output") == 0) {
			filter->param_previous_output = param;
		} else if (filter->transition && strcmp(info.name, "image_a") == 0) {
			filter->param_image_a = param;
		} else if (filter->transition && strcmp(info.name, "image_b") == 0) {
			filter->param_image_b = param;
		} else if (filter->transition && strcmp(info.name, "transition_time") == 0) {
			filter->param_transition_time = param;
		} else if (filter->transition && strcmp(info.name, "convert_linear") == 0) {
			filter->param_convert_linear = param;
		} else {
			struct effect_param_data *cached_data = da_push_back_new(filter->stored_param_list);
			dstr_copy(&cached_data->name, info.name);
			cached_data->type = info.type;
			cached_data->param = param;
			da_init(cached_data->option_values);
			da_init(cached_data->option_labels);
			const size_t annotation_count = gs_param_get_num_annotations(param);
			for (size_t annotation_index = 0; annotation_index < annotation_count; annotation_index++) {
				gs_eparam_t *annotation = gs_param_get_annotation_by_idx(param, annotation_index);
				void *annotation_default = gs_effect_get_default_val(annotation);
				gs_effect_get_param_info(annotation, &info);
				if (strcmp(info.name, "name") == 0 && info.type == GS_SHADER_PARAM_STRING) {
					dstr_copy(&cached_data->display_name, (const char *)annotation_default);
				} else if (strcmp(info.name, "label") == 0 && info.type == GS_SHADER_PARAM_STRING) {
					dstr_copy(&cached_data->display_name, (const char *)annotation_default);
				} else if (strcmp(info.name, "widget_type") == 0 && info.type == GS_SHADER_PARAM_STRING) {
					dstr_copy(&cached_data->widget_type, (const char *)annotation_default);
				} else if (strcmp(info.name, "group") == 0 && info.type == GS_SHADER_PARAM_STRING) {
					dstr_copy(&cached_data->group, (const char *)annotation_default);
				} else if (strcmp(info.name, "minimum") == 0) {
					if (info.type == GS_SHADER_PARAM_FLOAT || info.type == GS_SHADER_PARAM_VEC2 ||
					    info.type == GS_SHADER_PARAM_VEC3 || info.type == GS_SHADER_PARAM_VEC4) {
						cached_data->minimum.f = *(float *)annotation_default;
					} else if (info.type == GS_SHADER_PARAM_INT) {
						cached_data->minimum.i = *(int *)annotation_default;
					}
				} else if (strcmp(info.name, "maximum") == 0) {
					if (info.type == GS_SHADER_PARAM_FLOAT || info.type == GS_SHADER_PARAM_VEC2 ||
					    info.type == GS_SHADER_PARAM_VEC3 || info.type == GS_SHADER_PARAM_VEC4) {
						cached_data->maximum.f = *(float *)annotation_default;
					} else if (info.type == GS_SHADER_PARAM_INT) {
						cached_data->maximum.i = *(int *)annotation_default;
					}
				} else if (strcmp(info.name, "step") == 0) {
					if (info.type == GS_SHADER_PARAM_FLOAT || info.type == GS_SHADER_PARAM_VEC2 ||
					    info.type == GS_SHADER_PARAM_VEC3 || info.type == GS_SHADER_PARAM_VEC4) {
						cached_data->step.f = *(float *)annotation_default;
					} else if (info.type == GS_SHADER_PARAM_INT) {
						cached_data->step.i = *(int *)annotation_default;
					}
				} else if (strncmp(info.name, "option_", 7) == 0) {
					int id = atoi(info.name + 7);
					if (info.type == GS_SHADER_PARAM_INT) {
						int val = *(int *)annotation_default;
						int *cd = da_insert_new(cached_data->option_values, id);
						*cd = val;

					} else if (info.type == GS_SHADER_PARAM_STRING) {
						struct dstr val = {0};
						dstr_copy(&val, (const char *)annotation_default);
						struct dstr *cs = da_insert_new(cached_data->option_labels, id);
						*cs = val;
					}
				}
				bfree(annotation_default);
			}
		}
	}

end:
	obs_data_release(settings);
}
