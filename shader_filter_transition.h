// Version 3.0 by Chooka https://github.com/Chooka/obs-shaderfilter.git
// Version 2.0 by Exeldro https://github.com/exeldro/obs-shaderfilter
// Version 1.21 by Charles Fettinger https://github.com/Oncorporation/obs-shaderfilter
// original version by nleseul https://github.com/nleseul/obs-shaderfilter

#pragma once

#include <obs-module.h>
#include <graphics/graphics.h>

void *shader_transition_create(obs_data_t *settings, obs_source_t *source);
const char *shader_transition_get_name(void *unused);
bool shader_transition_audio_render(void *data, uint64_t *ts_out, struct obs_source_audio_mix *audio, uint32_t mixers, size_t channels, size_t sample_rate);
void shader_transition_defaults(obs_data_t *settings);
void shader_transition_video_render(void *data, gs_effect_t *effect);
void shader_transition_video_callback(void *data, gs_texture_t *a, gs_texture_t *b, float t, uint32_t cx, uint32_t cy);
enum gs_color_space shader_transition_get_color_space(void *data, size_t count, const enum gs_color_space *preferred_spaces);
float mix_a(void *data, float t);
static float mix_b(void *data, float t);
