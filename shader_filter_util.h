// Version 3.0 by Chooka https://github.com/Chooka/obs-shaderfilter.git
// Version 2.0 by Exeldro https://github.com/exeldro/obs-shaderfilter
// Version 1.21 by Charles Fettinger https://github.com/Oncorporation/obs-shaderfilter
// original version by nleseul https://github.com/nleseul/obs-shaderfilter

#pragma once

#include <obs-module.h>

unsigned int rand_interval(unsigned int min, unsigned int max);

bool add_source_to_list(void *data, obs_source_t *source);

bool is_var_char(char ch);

