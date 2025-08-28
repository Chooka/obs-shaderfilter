// Version 3.0 by Chooka https://github.com/Chooka/obs-shaderfilter.git
// Version 2.0 by Exeldro https://github.com/exeldro/obs-shaderfilter
// Version 1.21 by Charles Fettinger https://github.com/Oncorporation/obs-shaderfilter
// original version by nleseul https://github.com/nleseul/obs-shaderfilter

#pragma once


#include <obs-module.h>
#include <graphics/graphics.h>


void *shader_filter_create(obs_data_t *settings, obs_source_t *source);
void shader_filter_destroy(void *data);
void shader_filter_update(void *data, obs_data_t *settings);
void shader_filter_tick(void *data, float seconds);

void shader_filter_activate(void *data);
void shader_filter_deactivate(void *data);
void shader_filter_show(void *data);
void shader_filter_hide(void *data);

const char *shader_filter_get_name(void *unused);
void shader_filter_defaults(obs_data_t *settings);
uint32_t shader_filter_getwidth(void *data);
uint32_t shader_filter_getheight(void *data);
enum gs_color_space shader_filter_get_color_space(void *data, size_t count, const enum gs_color_space *preferred_spaces);

void load_output_effect(struct shader_filter_data *filter);
char *load_shader_from_file(const char *file_name);
void shader_filter_reload_effect(struct shader_filter_data *filter);


