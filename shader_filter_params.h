// Version 3.0 by Chooka https://github.com/Chooka/obs-shaderfilter.git
// Version 2.0 by Exeldro https://github.com/exeldro/obs-shaderfilter
// Version 1.21 by Charles Fettinger https://github.com/Oncorporation/obs-shaderfilter
// original version by nleseul https://github.com/nleseul/obs-shaderfilter

#pragma once

#include <obs-module.h>

void shader_filter_clear_params(struct shader_filter_data *filter);

void shader_filter_set_effect_params(struct shader_filter_data *filter);

void shader_filter_param_source_action(void *data, void (*action)(obs_source_t *source));
