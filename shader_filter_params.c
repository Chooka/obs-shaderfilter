// Version 3.0 by Chooka https://github.com/Chooka/obs-shaderfilter.git
// Version 2.0 by Exeldro https://github.com/exeldro/obs-shaderfilter
// Version 1.21 by Charles Fettinger https://github.com/Oncorporation/obs-shaderfilter
// original version by nleseul https://github.com/nleseul/obs-shaderfilter

#include "shader_filter_params.h"
#include "shader_filter_types.h"

#include <time.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

void shader_filter_clear_params(struct shader_filter_data *filter)
{
	filter->param_current_time_ms = NULL;
	filter->param_current_time_sec = NULL;
	filter->param_current_time_min = NULL;
	filter->param_current_time_hour = NULL;
	filter->param_current_time_day_of_week = NULL;
	filter->param_current_time_day_of_month = NULL;
	filter->param_current_time_month = NULL;
	filter->param_current_time_day_of_year = NULL;
	filter->param_current_time_year = NULL;
	filter->param_elapsed_time = NULL;
	filter->param_elapsed_time_start = NULL;
	filter->param_elapsed_time_show = NULL;
	filter->param_elapsed_time_active = NULL;
	filter->param_elapsed_time_enable = NULL;
	filter->param_uv_offset = NULL;
	filter->param_uv_pixel_interval = NULL;
	filter->param_uv_scale = NULL;
	filter->param_uv_size = NULL;
	filter->param_rand_f = NULL;
	filter->param_rand_activation_f = NULL;
	filter->param_rand_instance_f = NULL;
	filter->param_loops = NULL;
	filter->param_loop_second = NULL;
	filter->param_local_time = NULL;
	filter->param_image = NULL;
	filter->param_previous_image = NULL;
	filter->param_image_a = NULL;
	filter->param_image_b = NULL;
	filter->param_transition_time = NULL;
	filter->param_convert_linear = NULL;
	filter->param_previous_output = NULL;

	size_t param_count = filter->stored_param_list.num;
	for (size_t param_index = 0; param_index < param_count; param_index++) {
		struct effect_param_data *param = (filter->stored_param_list.array + param_index);
		if (param->image) {
			obs_enter_graphics();
			gs_image_file_free(param->image);
			obs_leave_graphics();
			bfree(param->image);
			param->image = NULL;
		}
		if (param->source) {
			obs_source_t *source = obs_weak_source_get_source(param->source);
			if (source) {
				if ((!filter->transition || filter->prev_transitioning) && obs_source_active(filter->context))
					obs_source_dec_active(source);
				if ((!filter->transition || filter->prev_transitioning) && obs_source_showing(filter->context))
					obs_source_dec_showing(source);
				obs_source_release(source);
			}
			obs_weak_source_release(param->source);
			param->source = NULL;
		}
		if (param->render) {
			obs_enter_graphics();
			gs_texrender_destroy(param->render);
			obs_leave_graphics();
			param->render = NULL;
		}
		dstr_free(&param->name);
		dstr_free(&param->display_name);
		dstr_free(&param->widget_type);
		dstr_free(&param->group);
		dstr_free(&param->path);
		da_free(param->option_values);
		for (size_t i = 0; i < param->option_labels.num; i++) {
			dstr_free(&param->option_labels.array[i]);
		}
		da_free(param->option_labels);
	}

	da_free(filter->stored_param_list);
}

void shader_filter_set_effect_params(struct shader_filter_data *filter)
{
	if (filter->param_uv_scale)
		gs_effect_set_vec2(filter->param_uv_scale, &filter->uv_scale);
	if (filter->param_uv_offset)
		gs_effect_set_vec2(filter->param_uv_offset, &filter->uv_offset);
	if (filter->param_uv_pixel_interval)
		gs_effect_set_vec2(filter->param_uv_pixel_interval, &filter->uv_pixel_interval);

#ifdef _WIN32
	SYSTEMTIME system_time;
	GetSystemTime(&system_time);
	if (filter->param_current_time_ms)
		gs_effect_set_int(filter->param_current_time_ms, system_time.wMilliseconds);
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);
	if (filter->param_current_time_ms)
		gs_effect_set_int(filter->param_current_time_ms, tv.tv_usec / 1000);
#endif

	time_t t = time(NULL);
	struct tm *lt = localtime(&t);
	if (filter->param_current_time_sec)
		gs_effect_set_int(filter->param_current_time_sec, lt->tm_sec);
	if (filter->param_current_time_min)
		gs_effect_set_int(filter->param_current_time_min, lt->tm_min);
	if (filter->param_current_time_hour)
		gs_effect_set_int(filter->param_current_time_hour, lt->tm_hour);
	if (filter->param_current_time_day_of_week)
		gs_effect_set_int(filter->param_current_time_day_of_week, lt->tm_wday);
	if (filter->param_current_time_day_of_month)
		gs_effect_set_int(filter->param_current_time_day_of_month, lt->tm_mday);
	if (filter->param_current_time_month)
		gs_effect_set_int(filter->param_current_time_month, lt->tm_mon);
	if (filter->param_current_time_day_of_year)
		gs_effect_set_int(filter->param_current_time_day_of_year, lt->tm_yday);
	if (filter->param_current_time_year)
		gs_effect_set_int(filter->param_current_time_year, lt->tm_year);

	if (filter->param_elapsed_time)
		gs_effect_set_float(filter->param_elapsed_time, filter->elapsed_time);
	if (filter->param_elapsed_time_start)
		gs_effect_set_float(filter->param_elapsed_time_start, filter->elapsed_time - filter->shader_start_time);
	if (filter->param_elapsed_time_show)
		gs_effect_set_float(filter->param_elapsed_time_show, filter->shader_show_time);
	if (filter->param_elapsed_time_active)
		gs_effect_set_float(filter->param_elapsed_time_active, filter->shader_active_time);
	if (filter->param_elapsed_time_enable)
		gs_effect_set_float(filter->param_elapsed_time_enable, filter->elapsed_time - filter->shader_enable_time);

	if (filter->param_uv_size)
		gs_effect_set_vec2(filter->param_uv_size, &filter->uv_size);
	if (filter->param_local_time)
		gs_effect_set_float(filter->param_local_time, filter->local_time);
	if (filter->param_loops)
		gs_effect_set_int(filter->param_loops, filter->loops);
	if (filter->param_loop_second)
		gs_effect_set_float(filter->param_loop_second, filter->elapsed_time_loop);
	if (filter->param_rand_f)
		gs_effect_set_float(filter->param_rand_f, filter->rand_f);
	if (filter->param_rand_activation_f)
		gs_effect_set_float(filter->param_rand_activation_f, filter->rand_activation_f);
	if (filter->param_rand_instance_f)
		gs_effect_set_float(filter->param_rand_instance_f, filter->rand_instance_f);

	size_t param_count = filter->stored_param_list.num;
	for (size_t i = 0; i < param_count; i++) {
		struct effect_param_data *param = &filter->stored_param_list.array[i];
		if (!param->param)
			continue;

		switch (param->type) {
		case GS_SHADER_PARAM_BOOL:
			gs_effect_set_bool(param->param, param->value.i);
			break;
		case GS_SHADER_PARAM_FLOAT:
			gs_effect_set_float(param->param, (float)param->value.f);
			break;
		case GS_SHADER_PARAM_INT:
			gs_effect_set_int(param->param, (int)param->value.i);
			break;
		case GS_SHADER_PARAM_VEC2:
			gs_effect_set_vec2(param->param, &param->value.vec2);
			break;
		case GS_SHADER_PARAM_VEC3:
			gs_effect_set_vec3(param->param, &param->value.vec3);
			break;
		case GS_SHADER_PARAM_VEC4:
			gs_effect_set_vec4(param->param, &param->value.vec4);
			break;
		case GS_SHADER_PARAM_STRING:
			gs_effect_set_val(param->param, param->value.string, gs_effect_get_val_size(param->param));
			break;
		default:
			break;
		}
	}
}

void shader_filter_param_source_action(void *data, void (*action)(obs_source_t *source))
{
	struct shader_filter_data *filter = data;
	size_t param_count = filter->stored_param_list.num;
	for (size_t param_index = 0; param_index < param_count; param_index++) {
		struct effect_param_data *param = (filter->stored_param_list.array + param_index);
		if (!param->source)
			continue;
		obs_source_t *source = obs_weak_source_get_source(param->source);
		if (!source)
			continue;
		action(source);
		obs_source_release(source);
	}
}
