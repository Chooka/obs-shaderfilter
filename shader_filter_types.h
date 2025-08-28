// Version 3.0 by Chooka https://github.com/Chooka/obs-shaderfilter.git
// Version 2.0 by Exeldro https://github.com/exeldro/obs-shaderfilter
// Version 1.21 by Charles Fettinger https://github.com/Oncorporation/obs-shaderfilter
// original version by nleseul https://github.com/nleseul/obs-shaderfilter

#pragma once

#include <obs-module.h>
#include <graphics/image-file.h>
#include <graphics/vec2.h>
#include <graphics/vec3.h>
#include <graphics/vec4.h>
#include <util/dstr.h>
#include <util/darray.h>

#define nullptr ((void *)0)

struct effect_param_data {
	struct dstr name;
	struct dstr display_name;
	struct dstr widget_type;
	struct dstr group;
	struct dstr path;

	DARRAY(int) option_values;
	DARRAY(struct dstr) option_labels;

	enum gs_shader_param_type type;
	gs_eparam_t *param;

	gs_image_file_t *image;
	gs_texrender_t *render;
	obs_weak_source_t *source;

	union {
		long long i;
		double f;
		char *string;
		struct vec2 vec2;
		struct vec3 vec3;
		struct vec4 vec4;
	} value;

	union {
		long long i;
		double f;
		char *string;
		struct vec2 vec2;
		struct vec3 vec3;
		struct vec4 vec4;
	} default_value;

	bool has_default;
	char *label;

	union {
		long long i;
		double f;
	} minimum;
	union {
		long long i;
		double f;
	} maximum;
	union {
		long long i;
		double f;
	} step;
};

struct shader_filter_data {
	obs_source_t *context;
	gs_effect_t *effect;
	gs_effect_t *output_effect;
	gs_vertbuffer_t *sprite_buffer;

	gs_texrender_t *input_texrender;
	gs_texrender_t *previous_input_texrender;
	gs_texrender_t *output_texrender;
	gs_texrender_t *previous_output_texrender;
	gs_eparam_t *param_output_image;

	bool reload_effect;
	struct dstr last_path;
	bool last_from_file;
	bool transition;
	bool transitioning;
	bool prev_transitioning;

	bool use_pm_alpha;
	bool output_rendered;
	bool input_rendered;

	float shader_start_time;
	float shader_show_time;
	float shader_active_time;
	float shader_enable_time;
	bool enabled;
	bool use_template;

	gs_eparam_t *param_uv_offset;
	gs_eparam_t *param_uv_scale;
	gs_eparam_t *param_uv_pixel_interval;
	gs_eparam_t *param_uv_size;
	gs_eparam_t *param_current_time_ms;
	gs_eparam_t *param_current_time_sec;
	gs_eparam_t *param_current_time_min;
	gs_eparam_t *param_current_time_hour;
	gs_eparam_t *param_current_time_day_of_week;
	gs_eparam_t *param_current_time_day_of_month;
	gs_eparam_t *param_current_time_month;
	gs_eparam_t *param_current_time_day_of_year;
	gs_eparam_t *param_current_time_year;
	gs_eparam_t *param_elapsed_time;
	gs_eparam_t *param_elapsed_time_start;
	gs_eparam_t *param_elapsed_time_show;
	gs_eparam_t *param_elapsed_time_active;
	gs_eparam_t *param_elapsed_time_enable;
	gs_eparam_t *param_loops;
	gs_eparam_t *param_loop_second;
	gs_eparam_t *param_local_time;
	gs_eparam_t *param_rand_f;
	gs_eparam_t *param_rand_instance_f;
	gs_eparam_t *param_rand_activation_f;
	gs_eparam_t *param_image;
	gs_eparam_t *param_previous_image;
	gs_eparam_t *param_image_a;
	gs_eparam_t *param_image_b;
	gs_eparam_t *param_transition_time;
	gs_eparam_t *param_convert_linear;
	gs_eparam_t *param_previous_output;

	int expand_left;
	int expand_right;
	int expand_top;
	int expand_bottom;

	int total_width;
	int total_height;
	bool no_repeat;
	bool rendering;

	struct vec2 uv_offset;
	struct vec2 uv_scale;
	struct vec2 uv_pixel_interval;
	struct vec2 uv_size;
	float elapsed_time;
	float elapsed_time_loop;
	int loops;
	float local_time;
	float rand_f;
	float rand_instance_f;
	float rand_activation_f;

	DARRAY(struct effect_param_data) stored_param_list;
};
