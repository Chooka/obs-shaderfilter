// Version 3.0 by Chooka https://github.com/Chooka/obs-shaderfilter.git
// Version 2.0 by Exeldro https://github.com/exeldro/obs-shaderfilter
// Version 1.21 by Charles Fettinger https://github.com/Oncorporation/obs-shaderfilter
// original version by nleseul https://github.com/nleseul/obs-shaderfilter

#include "shader_filter_props.h"
#include "shader_filter_types.h"
#include "shader_filter_util.h"
#include "shader_filter_convert.h"

#include "version.h"

#include <util/platform.h>

const char *shader_filter_texture_file_filter = "Textures (*.bmp *.tga *.png *.jpeg *.jpg *.gif);;";

bool shader_filter_reload_effect_clicked(obs_properties_t *props, obs_property_t *property, void *data)
{
	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(property);
	struct shader_filter_data *filter = data;

	filter->reload_effect = true;
	obs_source_update(filter->context, NULL);

	return false;
}

bool shader_filter_from_file_changed(obs_properties_t *props, obs_property_t *p, obs_data_t *settings)
{
	UNUSED_PARAMETER(p);
	struct shader_filter_data *filter = obs_properties_get_param(props);

	bool from_file = obs_data_get_bool(settings, "from_file");

	obs_property_set_visible(obs_properties_get(props, "shader_text"), !from_file);
	obs_property_set_visible(obs_properties_get(props, "shader_file_name"), from_file);

	if (from_file != filter->last_from_file) {
		filter->reload_effect = true;
	}
	filter->last_from_file = from_file;

	return true;
}

bool shader_filter_text_changed(obs_properties_t *props, obs_property_t *p, obs_data_t *settings)
{
	UNUSED_PARAMETER(p);
	struct shader_filter_data *filter = obs_properties_get_param(props);
	if (!filter)
		return false;

	const char *shader_text = obs_data_get_string(settings, "shader_text");
	bool can_convert = strstr(shader_text, "void mainImage( out vec4") || strstr(shader_text, "void mainImage(out vec4") ||
			   strstr(shader_text, "void main()") || strstr(shader_text, "vec4 effect(vec4");
	obs_property_t *shader_convert = obs_properties_get(props, "shader_convert");
	bool visible = obs_property_visible(obs_properties_get(props, "shader_text"));
	if (obs_property_visible(shader_convert) != (can_convert && visible)) {
		obs_property_set_visible(shader_convert, can_convert && visible);
		return true;
	}
	return false;
}

bool shader_filter_file_name_changed(obs_properties_t *props, obs_property_t *p, obs_data_t *settings)
{
	struct shader_filter_data *filter = obs_properties_get_param(props);
	const char *new_file_name = obs_data_get_string(settings, obs_property_name(p));

	if ((dstr_is_empty(&filter->last_path) && strlen(new_file_name)) ||
	    (filter->last_path.array && dstr_cmp(&filter->last_path, new_file_name) != 0)) {
		filter->reload_effect = true;
		dstr_copy(&filter->last_path, new_file_name);
		size_t l = strlen(new_file_name);
		if (l > 7 && strncmp(new_file_name + l - 7, ".effect", 7) == 0) {
			obs_data_set_bool(settings, "override_entire_effect", true);
		} else if (l > 7 && strncmp(new_file_name + l - 7, ".shader", 7) == 0) {
			obs_data_set_bool(settings, "override_entire_effect", false);
		}
	}

	return false;
}

obs_properties_t *shader_filter_properties(void *data)
{
	struct shader_filter_data *filter = data;

	struct dstr examples_path = {0};
	dstr_init(&examples_path);
	dstr_cat(&examples_path, obs_get_module_data_path(obs_current_module()));
	dstr_cat(&examples_path, "/examples");

	obs_properties_t *props = obs_properties_create();
	obs_properties_set_param(props, filter, NULL);

	if (!filter || !filter->transition) {
		obs_properties_add_int(props, "expand_left", obs_module_text("ShaderFilter.ExpandLeft"), 0, 9999, 1);
		obs_properties_add_int(props, "expand_right", obs_module_text("ShaderFilter.ExpandRight"), 0, 9999, 1);
		obs_properties_add_int(props, "expand_top", obs_module_text("ShaderFilter.ExpandTop"), 0, 9999, 1);
		obs_properties_add_int(props, "expand_bottom", obs_module_text("ShaderFilter.ExpandBottom"), 0, 9999, 1);
	}

	obs_properties_add_bool(props, "override_entire_effect", obs_module_text("ShaderFilter.OverrideEntireEffect"));

	obs_property_t *from_file = obs_properties_add_bool(props, "from_file", obs_module_text("ShaderFilter.LoadFromFile"));
	obs_property_set_modified_callback(from_file, shader_filter_from_file_changed);

	obs_property_t *shader_text =
		obs_properties_add_text(props, "shader_text", obs_module_text("ShaderFilter.ShaderText"), OBS_TEXT_MULTILINE);
	obs_property_set_modified_callback(shader_text, shader_filter_text_changed);

	obs_properties_add_button2(props, "shader_convert", obs_module_text("ShaderFilter.Convert"), shader_filter_convert, data);

	char *abs_path = os_get_abs_path_ptr(examples_path.array);
	obs_property_t *file_name = obs_properties_add_path(props, "shader_file_name",
							    obs_module_text("ShaderFilter.ShaderFileName"), OBS_PATH_FILE, NULL,
							    abs_path ? abs_path : examples_path.array);
	if (abs_path)
		bfree(abs_path);
	dstr_free(&examples_path);
	obs_property_set_modified_callback(file_name, shader_filter_file_name_changed);

	if (filter) {
		obs_data_t *settings = obs_source_get_settings(filter->context);
		const char *last_error = obs_data_get_string(settings, "last_error");
		if (last_error && strlen(last_error)) {
			obs_property_t *error =
				obs_properties_add_text(props, "last_error", obs_module_text("ShaderFilter.Error"), OBS_TEXT_INFO);
			obs_property_text_set_info_type(error, OBS_TEXT_INFO_ERROR);
		}
		obs_data_release(settings);
	}

	obs_properties_add_button(props, "reload_effect", obs_module_text("ShaderFilter.ReloadEffect"),
				  shader_filter_reload_effect_clicked);

	DARRAY(obs_property_t *) groups;
	da_init(groups);

	size_t param_count = filter->stored_param_list.num;
	for (size_t param_index = 0; param_index < param_count; param_index++) {
		struct effect_param_data *param = (filter->stored_param_list.array + param_index);
		//gs_eparam_t *annot = gs_param_get_annotation_by_idx(param->param, param_index);
		const char *param_name = param->name.array;
		const char *label = param->display_name.array;
		const char *widget_type = param->widget_type.array;
		const char *group_name = param->group.array;
		const int *options = param->option_values.array;
		const struct dstr *option_labels = param->option_labels.array;

		struct dstr display_name = {0};
		struct dstr sources_name = {0};

		if (label == NULL) {
			dstr_ncat(&display_name, param_name, param->name.len);
			dstr_replace(&display_name, "_", " ");
		} else {
			dstr_ncat(&display_name, label, param->display_name.len);
		}
		obs_properties_t *group = NULL;
		if (group_name && strlen(group_name)) {
			for (size_t i = 0; i < groups.num; i++) {
				const char *n = obs_property_name(groups.array[i]);
				if (strcmp(n, group_name) == 0) {
					group = obs_property_group_content(groups.array[i]);
				}
			}
			if (!group) {
				group = obs_properties_create();
				obs_property_t *p =
					obs_properties_add_group(props, group_name, group_name, OBS_GROUP_NORMAL, group);
				da_push_back(groups, &p);
			}
		}
		if (!group)
			group = props;
		switch (param->type) {
		case GS_SHADER_PARAM_BOOL:
			obs_properties_add_bool(group, param_name, display_name.array);
			break;
		case GS_SHADER_PARAM_FLOAT: {
			double range_min = param->minimum.f;
			double range_max = param->maximum.f;
			double step = param->step.f;
			if (range_min == range_max) {
				range_min = -1000.0;
				range_max = 1000.0;
				step = 0.0001;
			}
			obs_properties_remove_by_name(props, param_name);
			if (widget_type != NULL && strcmp(widget_type, "slider") == 0) {
				obs_properties_add_float_slider(group, param_name, display_name.array, range_min, range_max, step);
			} else {
				obs_properties_add_float(group, param_name, display_name.array, range_min, range_max, step);
			}
			break;
		}
		case GS_SHADER_PARAM_INT: {
			int range_min = (int)param->minimum.i;
			int range_max = (int)param->maximum.i;
			int step = (int)param->step.i;
			if (range_min == range_max) {
				range_min = -1000;
				range_max = 1000;
				step = 1;
			}
			obs_properties_remove_by_name(props, param_name);

			if (widget_type != NULL && strcmp(widget_type, "slider") == 0) {
				obs_properties_add_int_slider(group, param_name, display_name.array, range_min, range_max, step);
			} else if (widget_type != NULL && strcmp(widget_type, "select") == 0) {
				obs_property_t *plist = obs_properties_add_list(group, param_name, display_name.array,
										OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
				for (size_t i = 0; i < param->option_values.num; i++) {
					obs_property_list_add_int(plist, option_labels[i].array, options[i]);
				}
			} else {
				obs_properties_add_int(group, param_name, display_name.array, range_min, range_max, step);
			}
			break;
		}
		case GS_SHADER_PARAM_INT3:

			break;
		case GS_SHADER_PARAM_VEC2: {
			double range_min = param->minimum.f;
			double range_max = param->maximum.f;
			double step = param->step.f;
			if (range_min == range_max) {
				range_min = -1000.0;
				range_max = 1000.0;
				step = 0.0001;
			}

			bool slider = (widget_type != NULL && strcmp(widget_type, "slider") == 0);

			for (size_t i = 0; i < 2; i++) {
				dstr_printf(&sources_name, "%s_%zu", param_name, i);
				if (i < param->option_labels.num) {
					if (slider) {
						obs_properties_add_float_slider(group, sources_name.array,
										param->option_labels.array[i].array, range_min,
										range_max, step);
					} else {
						obs_properties_add_float(group, sources_name.array,
									 param->option_labels.array[i].array, range_min, range_max,
									 step);
					}
				} else if (slider) {

					obs_properties_add_float_slider(group, sources_name.array, display_name.array, range_min,
									range_max, step);
				} else {
					obs_properties_add_float(group, sources_name.array, display_name.array, range_min,
								 range_max, step);
				}
			}
			dstr_free(&sources_name);

			break;
		}
		case GS_SHADER_PARAM_VEC3:
			if (widget_type != NULL && strcmp(widget_type, "slider") == 0) {
				double range_min = param->minimum.f;
				double range_max = param->maximum.f;
				double step = param->step.f;
				if (range_min == range_max) {
					range_min = -1000.0;
					range_max = 1000.0;
					step = 0.0001;
				}
				for (size_t i = 0; i < 3; i++) {
					dstr_printf(&sources_name, "%s_%zu", param_name, i);
					if (i < param->option_labels.num) {
						obs_properties_add_float_slider(group, sources_name.array,
										param->option_labels.array[i].array, range_min,
										range_max, step);
					} else {
						obs_properties_add_float_slider(group, sources_name.array, display_name.array,
										range_min, range_max, step);
					}
				}
				dstr_free(&sources_name);
			} else {
				obs_properties_add_color(group, param_name, display_name.array);
			}
			break;
		case GS_SHADER_PARAM_VEC4:
			if (widget_type != NULL && strcmp(widget_type, "slider") == 0) {
				double range_min = param->minimum.f;
				double range_max = param->maximum.f;
				double step = param->step.f;
				if (range_min == range_max) {
					range_min = -1000.0;
					range_max = 1000.0;
					step = 0.0001;
				}
				for (size_t i = 0; i < 4; i++) {
					dstr_printf(&sources_name, "%s_%zu", param_name, i);
					if (i < param->option_labels.num) {
						obs_properties_add_float_slider(group, sources_name.array,
										param->option_labels.array[i].array, range_min,
										range_max, step);
					} else {
						obs_properties_add_float_slider(group, sources_name.array, display_name.array,
										range_min, range_max, step);
					}
				}
				dstr_free(&sources_name);
			} else {
				obs_properties_add_color_alpha(group, param_name, display_name.array);
			}
			break;
		case GS_SHADER_PARAM_TEXTURE:
			if (widget_type != NULL && strcmp(widget_type, "source") == 0) {
				dstr_init_copy_dstr(&sources_name, &param->name);
				dstr_cat(&sources_name, "_source");
				obs_property_t *p = obs_properties_add_list(group, sources_name.array, display_name.array,
									    OBS_COMBO_TYPE_EDITABLE, OBS_COMBO_FORMAT_STRING);
				dstr_free(&sources_name);
				obs_enum_sources(add_source_to_list, p);
				obs_enum_scenes(add_source_to_list, p);
				obs_property_list_insert_string(p, 0, "", "");

			} else if (widget_type != NULL && strcmp(widget_type, "file") == 0) {
				obs_properties_add_path(group, param_name, display_name.array, OBS_PATH_FILE,
							shader_filter_texture_file_filter, NULL);
			} else {
				dstr_init_copy_dstr(&sources_name, &param->name);
				dstr_cat(&sources_name, "_source");
				obs_property_t *p = obs_properties_add_list(group, sources_name.array, display_name.array,
									    OBS_COMBO_TYPE_EDITABLE, OBS_COMBO_FORMAT_STRING);
				dstr_free(&sources_name);
				obs_property_list_add_string(p, "", "");
				obs_enum_sources(add_source_to_list, p);
				obs_enum_scenes(add_source_to_list, p);
				obs_properties_add_path(group, param_name, display_name.array, OBS_PATH_FILE,
							shader_filter_texture_file_filter, NULL);
			}
			break;
		case GS_SHADER_PARAM_STRING:
			if (widget_type != NULL && strcmp(widget_type, "info") == 0) {
				obs_properties_add_text(group, param_name, display_name.array, OBS_TEXT_INFO);
			} else {
				obs_properties_add_text(group, param_name, display_name.array, OBS_TEXT_MULTILINE);
			}
			break;
		default:;
		}
		dstr_free(&display_name);
	}
	da_free(groups);

	obs_properties_add_text(
		props, "plugin_info",
		"<a href=\"https://obsproject.com/forum/resources/obs-shaderfilter.1736/\">obs-shaderfilter</a> (" PROJECT_VERSION
		") by <a href=\"https://www.exeldro.com\">Exeldro</a> refactored by <a href=\"https://github.com/Chooka/obs-shaderfilter\">Chooka</a>",
		OBS_TEXT_INFO);
	return props;
}
