// Version 3.0 by Chooka https://github.com/Chooka/obs-shaderfilter.git
// Version 2.0 by Exeldro https://github.com/exeldro/obs-shaderfilter
// Version 1.21 by Charles Fettinger https://github.com/Oncorporation/obs-shaderfilter
// original version by nleseul https://github.com/nleseul/obs-shaderfilter

#include "shader_filter_render.h"
#include "shader_filter_types.h"
#include "shader_filter_main.h"
#include "shader_filter_params.h"
#include "shader_filter_textures.h"

extern float (*move_get_transition_filter)(obs_source_t *filter_from, obs_source_t **filter_to);

void get_input_source(struct shader_filter_data *filter)
{
	if (filter->input_rendered)
		return;

	// Use the OBS default effect file as our effect.
	gs_effect_t *pass_through = obs_get_base_effect(OBS_EFFECT_DEFAULT);

	// Set up our color space info.
	const enum gs_color_space preferred_spaces[] = {
		GS_CS_SRGB,
		GS_CS_SRGB_16F,
		GS_CS_709_EXTENDED,
	};

	const enum gs_color_space source_space =
		obs_source_get_color_space(obs_filter_get_target(filter->context), OBS_COUNTOF(preferred_spaces), preferred_spaces);

	const enum gs_color_format format = gs_get_format_from_space(source_space);

	if (filter->param_previous_image) {
		gs_texrender_t *temp = filter->input_texrender;
		filter->input_texrender = filter->previous_input_texrender;
		filter->previous_input_texrender = temp;
	}

	// Set up our input_texrender to catch the output texture.
	filter->input_texrender = create_or_reset_texrender(filter->input_texrender);

	// Start the rendering process with our correct color space params,
	// And set up your texrender to recieve the created texture.
	if (!filter->transition &&
	    !obs_source_process_filter_begin_with_color_space(filter->context, format, source_space, OBS_NO_DIRECT_RENDERING))
		return;

	if (gs_texrender_begin(filter->input_texrender, filter->total_width, filter->total_height)) {

		gs_blend_state_push();
		gs_reset_blend_state();
		gs_enable_blending(false);
		gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);

		gs_ortho(0.0f, (float)filter->total_width, 0.0f, (float)filter->total_height, -100.0f, 100.0f);
		// The incoming source is pre-multiplied alpha, so use the
		// OBS default effect "DrawAlphaDivide" technique to convert
		// the colors back into non-pre-multiplied space. If the shader
		// file has #define USE_PM_ALPHA 1, then use normal "Draw"
		// technique.
		const char *technique = filter->use_pm_alpha ? "Draw" : "DrawAlphaDivide";
		if (!filter->transition)
			obs_source_process_filter_tech_end(filter->context, pass_through, filter->total_width, filter->total_height,
							   technique);
		gs_texrender_end(filter->input_texrender);
		gs_blend_state_pop();
		filter->input_rendered = true;
	}
}


void render_shader(struct shader_filter_data *filter, float f, obs_source_t *filter_to)
{
	gs_texture_t *texture = gs_texrender_get_texture(filter->input_texrender);
	if (!texture) {
		return;
	}

	if (filter->param_previous_output) {
		gs_texrender_t *temp = filter->output_texrender;
		filter->output_texrender = filter->previous_output_texrender;
		filter->previous_output_texrender = temp;
	}
	filter->output_texrender = create_or_reset_texrender(filter->output_texrender);

	if (filter->param_image)
		gs_effect_set_texture(filter->param_image, texture);
	if (filter->param_previous_image)
		gs_effect_set_texture(filter->param_previous_image, gs_texrender_get_texture(filter->previous_input_texrender));
	if (filter->param_previous_output)
		gs_effect_set_texture(filter->param_previous_output, gs_texrender_get_texture(filter->previous_output_texrender));

	shader_filter_set_effect_params(filter);

	if (f > 0.0f) {
		if (filter_to) {

			struct shader_filter_data *filter2 = obs_obj_get_data(filter_to);
			for (size_t i = 0; i < filter->stored_param_list.num; i++) {
				struct effect_param_data *param = (filter->stored_param_list.array + i);
				if (!param->param)
					continue;

				for (size_t j = 0; j < filter->stored_param_list.num; j++) {
					struct effect_param_data *param2 = (filter2->stored_param_list.array + i);
					if (!param2->param)
						continue;
					if (param->type != param2->type)
						continue;
					if (strcmp(param->name.array, param2->name.array) != 0)
						continue;

					switch (param->type) {
					case GS_SHADER_PARAM_FLOAT:
						gs_effect_set_float(param->param, (float)param2->value.f * f +
											  (float)param->value.f * (1.0f - f));
						break;
					case GS_SHADER_PARAM_INT:
						gs_effect_set_int(param->param, (int)((double)param2->value.i * f +
										      (double)param->value.i * (1.0f - f)));
						break;
					case GS_SHADER_PARAM_VEC2: {
						struct vec2 v2;
						v2.x = (float)param2->value.vec2.x * f + (float)param->value.vec2.x * (1.0f - f);
						v2.y = (float)param2->value.vec2.y * f + (float)param->value.vec2.y * (1.0f - f);
						gs_effect_set_vec2(param->param, &v2);
						break;
					}
					case GS_SHADER_PARAM_VEC3: {
						struct vec3 v3;
						v3.x = (float)param2->value.vec3.x * f + (float)param->value.vec3.x * (1.0f - f);
						v3.y = (float)param2->value.vec3.y * f + (float)param->value.vec3.y * (1.0f - f);
						v3.z = (float)param2->value.vec3.z * f + (float)param->value.vec3.z * (1.0f - f);
						gs_effect_set_vec3(param->param, &v3);
						break;
					}
					case GS_SHADER_PARAM_VEC4: {
						struct vec4 v4;
						v4.x = (float)param2->value.vec4.x * f + (float)param->value.vec4.x * (1.0f - f);
						v4.y = (float)param2->value.vec4.y * f + (float)param->value.vec4.y * (1.0f - f);
						v4.z = (float)param2->value.vec4.z * f + (float)param->value.vec4.z * (1.0f - f);
						v4.w = (float)param2->value.vec4.w * f + (float)param->value.vec4.w * (1.0f - f);
						gs_effect_set_vec4(param->param, &v4);
						break;
					}
					default:;
					}
					break;
				}
			}
		} else {
			for (size_t i = 0; i < filter->stored_param_list.num; i++) {
				struct effect_param_data *param = (filter->stored_param_list.array + i);
				if (!param->param || !param->has_default)
					continue;

				switch (param->type) {
				case GS_SHADER_PARAM_FLOAT:
					gs_effect_set_float(param->param,
							    (float)param->default_value.f * f + (float)param->value.f * (1.0f - f));
					break;
				case GS_SHADER_PARAM_INT:
					gs_effect_set_int(param->param, (int)((double)param->default_value.i * f +
									      (double)param->value.i * (1.0f - f)));
					break;
				case GS_SHADER_PARAM_VEC2: {
					struct vec2 v2;
					v2.x = param->default_value.vec2.x * f + param->value.vec2.x * (1.0f - f);
					v2.y = param->default_value.vec2.y * f + param->value.vec2.y * (1.0f - f);
					gs_effect_set_vec2(param->param, &v2);
					break;
				}
				case GS_SHADER_PARAM_VEC3: {
					struct vec3 v3;
					v3.x = param->default_value.vec3.x * f + param->value.vec3.x * (1.0f - f);
					v3.y = param->default_value.vec3.y * f + param->value.vec3.y * (1.0f - f);
					v3.z = param->default_value.vec3.z * f + param->value.vec3.z * (1.0f - f);
					gs_effect_set_vec3(param->param, &v3);
					break;
				}
				case GS_SHADER_PARAM_VEC4: {
					struct vec4 v4;
					v4.x = param->default_value.vec4.x * f + param->value.vec4.x * (1.0f - f);
					v4.y = param->default_value.vec4.y * f + param->value.vec4.y * (1.0f - f);
					v4.z = param->default_value.vec4.z * f + param->value.vec4.z * (1.0f - f);
					v4.w = param->default_value.vec4.w * f + param->value.vec4.w * (1.0f - f);
					gs_effect_set_vec4(param->param, &v4);
					break;
				}
				default:;
				}
			}
		}
	}

	gs_blend_state_push();
	gs_reset_blend_state();
	gs_enable_blending(false);
	gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);

	if (gs_texrender_begin(filter->output_texrender, filter->total_width, filter->total_height)) {
		gs_ortho(0.0f, (float)filter->total_width, 0.0f, (float)filter->total_height, -100.0f, 100.0f);
		while (gs_effect_loop(filter->effect, "Draw")) {
			if (filter->use_template) {
				gs_draw_sprite(texture, 0, filter->total_width, filter->total_height);
			} else {
				if (!filter->sprite_buffer)
					load_sprite_buffer(filter);

				struct gs_vb_data *data = gs_vertexbuffer_get_data(filter->sprite_buffer);
				build_sprite_norm(data, (float)filter->total_width, (float)filter->total_height);
				gs_vertexbuffer_flush(filter->sprite_buffer);
				gs_load_vertexbuffer(filter->sprite_buffer);
				gs_load_indexbuffer(NULL);
				gs_draw(GS_TRISTRIP, 0, 0);
			}
		}
		gs_texrender_end(filter->output_texrender);
	}

	gs_blend_state_pop();
}

void draw_output(struct shader_filter_data *filter)
{
	const enum gs_color_space preferred_spaces[] = {
		GS_CS_SRGB,
		GS_CS_SRGB_16F,
		GS_CS_709_EXTENDED,
	};

	const enum gs_color_space source_space =
		obs_source_get_color_space(obs_filter_get_target(filter->context), OBS_COUNTOF(preferred_spaces), preferred_spaces);

	const enum gs_color_format format = gs_get_format_from_space(source_space);

	if (!obs_source_process_filter_begin_with_color_space(filter->context, format, source_space, OBS_NO_DIRECT_RENDERING)) {
		return;
	}

	gs_texture_t *texture = gs_texrender_get_texture(filter->output_texrender);
	gs_effect_t *pass_through = filter->output_effect;
	if (!pass_through)
		pass_through = obs_get_base_effect(OBS_EFFECT_DEFAULT);

	if (filter->param_output_image) {
		gs_effect_set_texture(filter->param_output_image, texture);
	}

	obs_source_process_filter_end(filter->context, pass_through, filter->total_width, filter->total_height);
}

void load_sprite_buffer(struct shader_filter_data *filter)
{
	if (filter->sprite_buffer)
		return;
	struct gs_vb_data *vbd = gs_vbdata_create();
	vbd->num = 4;
	vbd->points = bmalloc(sizeof(struct vec3) * 4);
	vbd->num_tex = 1;
	vbd->tvarray = bmalloc(sizeof(struct gs_tvertarray));
	vbd->tvarray[0].width = 2;
	vbd->tvarray[0].array = bmalloc(sizeof(struct vec2) * 4);
	memset(vbd->points, 0, sizeof(struct vec3) * 4);
	memset(vbd->tvarray[0].array, 0, sizeof(struct vec2) * 4);
	filter->sprite_buffer = gs_vertexbuffer_create(vbd, GS_DYNAMIC);
}

void build_sprite(struct gs_vb_data *data, float fcx, float fcy, float start_u, float end_u, float start_v, float end_v)
{
	struct vec2 *tvarray = data->tvarray[0].array;

	vec3_zero(data->points);
	vec3_set(data->points + 1, fcx, 0.0f, 0.0f);
	vec3_set(data->points + 2, 0.0f, fcy, 0.0f);
	vec3_set(data->points + 3, fcx, fcy, 0.0f);
	vec2_set(tvarray, start_u, start_v);
	vec2_set(tvarray + 1, end_u, start_v);
	vec2_set(tvarray + 2, start_u, end_v);
	vec2_set(tvarray + 3, end_u, end_v);
}

gs_texrender_t *create_or_reset_texrender(gs_texrender_t *render)
{
	if (!render) {
		render = gs_texrender_create(GS_RGBA, GS_ZS_NONE);
	} else {
		gs_texrender_reset(render);
	}
	return render;
}

void shader_filter_render(void *data, gs_effect_t *effect)
{
	UNUSED_PARAMETER(effect);

	struct shader_filter_data *filter = data;

	float f = 0.0f;
	obs_source_t *filter_to = NULL;
	if (move_get_transition_filter)
		f = move_get_transition_filter(filter->context, &filter_to);

	if (f == 0.0f && filter->output_rendered) {
		draw_output(filter);
		return;
	}

	if (filter->effect == NULL || filter->rendering) {
		obs_source_skip_video_filter(filter->context);
		return;
	}

	get_input_source(filter);

	filter->rendering = true;
	render_shader(filter, f, filter_to);
	draw_output(filter);
	if (f == 0.0f)
		filter->output_rendered = true;
	filter->rendering = false;
}
