// Version 3.0 by Chooka https://github.com/Chooka/obs-shaderfilter.git
// Version 2.0 by Exeldro https://github.com/exeldro/obs-shaderfilter
// Version 1.21 by Charles Fettinger https://github.com/Oncorporation/obs-shaderfilter
// original version by nleseul https://github.com/nleseul/obs-shaderfilter

#pragma once

#include <obs-module.h>

void convert_if_defined(struct dstr *effect_text);

void convert_atan(struct dstr *effect_text);

void insert_begin_of_block(struct dstr *effect_text, size_t diff, char *insert);

void insert_end_of_block(struct dstr *effect_text, size_t diff, char *insert);

void convert_mat_mul_var(struct dstr *effect_text, struct dstr *var_function_name);

void convert_mat_mul(struct dstr *effect_text, char *var_type);

bool is_in_function(struct dstr *effect_text, size_t diff);

void convert_init(struct dstr *effect_text, char *name);

void convert_vector_init(struct dstr *effect_text, char *name, int count);

void convert_if0(struct dstr *effect_text);

void convert_if1(struct dstr *effect_text);

void convert_define(struct dstr *effect_text);

void convert_return(struct dstr *effect_text, struct dstr *var_name, size_t main_diff);

bool shader_filter_convert(obs_properties_t *props, obs_property_t *property, void *data);

