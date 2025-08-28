// Version 3.0 by Chooka https://github.com/Chooka/obs-shaderfilter.git
// Version 2.0 by Exeldro https://github.com/exeldro/obs-shaderfilter
// Version 1.21 by Charles Fettinger https://github.com/Oncorporation/obs-shaderfilter
// original version by nleseul https://github.com/nleseul/obs-shaderfilter

#pragma once

#include <obs-module.h>

void get_input_source(struct shader_filter_data *filter);

void render_shader(struct shader_filter_data *filter, float f, obs_source_t *filter_to);

void draw_output(struct shader_filter_data *filter);

void load_sprite_buffer(struct shader_filter_data *filter);

void build_sprite(struct gs_vb_data *data, float fcx, float fcy, float start_u, float end_u, float start_v, float end_v);

inline void build_sprite_norm(struct gs_vb_data *data, float fcx, float fcy)
{
	build_sprite(data, fcx, fcy, 0.0f, 1.0f, 0.0f, 1.0f);
}

gs_texrender_t *create_or_reset_texrender(gs_texrender_t *render);

void shader_filter_render(void *data, gs_effect_t *effect);
