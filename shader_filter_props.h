// Version 3.0 by Chooka https://github.com/Chooka/obs-shaderfilter.git
// Version 2.0 by Exeldro https://github.com/exeldro/obs-shaderfilter
// Version 1.21 by Charles Fettinger https://github.com/Oncorporation/obs-shaderfilter
// original version by nleseul https://github.com/nleseul/obs-shaderfilter

#pragma once

#include <obs-module.h>

bool shader_filter_reload_effect_clicked(obs_properties_t *props, obs_property_t *property, void *data);

bool shader_filter_from_file_changed(obs_properties_t *props, obs_property_t *p, obs_data_t *settings);

bool shader_filter_text_changed(obs_properties_t *props, obs_property_t *p, obs_data_t *settings);

bool shader_filter_file_name_changed(obs_properties_t *props, obs_property_t *p, obs_data_t *settings);

obs_properties_t *shader_filter_properties(void *data);
