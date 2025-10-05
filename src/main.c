#include <bresmon.h>
#include <bmacro.h>
#include <cute.h>
#include <cimgui.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <byteshift_memmem.h>

#define DECORATOR_PREFIX "// @param "
#define DECORATOR_LEN (sizeof(DECORATOR_PREFIX) - 1)

typedef struct {
	const char* str;
	size_t len;
} str_t;

typedef struct {
	size_t pos;
	const str_t source;
} parse_state_t;

typedef enum {
	DATA_SOURCE_UI = 0,
	DATA_SOURCE_TIME,
	DATA_SOURCE_DELTA_TIME,
	DATA_SOURCE_SCREEN_W,
	DATA_SOURCE_SCREEN_H,
} data_source_t;

typedef enum {
	PARAM_TARGET_UNIFORM,
	PARAM_TARGET_ATTR_X,
	PARAM_TARGET_ATTR_Y,
	PARAM_TARGET_ATTR_Z,
	PARAM_TARGET_ATTR_W,
} param_target_t;

typedef enum {
	SHADER_MODE_OFF,
	SHADER_MODE_SPRITE,
	SHADER_MODE_SCREEN,
} shader_mode_t;

typedef struct {
	const char* name;
	CF_UniformType type;
	bool edit_as_color;
	param_target_t target;
	data_source_t source;
	float min;
	float max;
	float step;
	union {
		int int_value[4];
		float float_value[4];
	} data;
} shader_param_t;

static CF_Shader current_shader = { 0 };
static htbl shader_param_t* previous_params = NULL;
static htbl shader_param_t* current_params = NULL;

static bool
next_line(parse_state_t* state, str_t* line) {
	if (state->pos >= state->source.len) { return false; }

	line->str = state->source.str + state->pos;
	line->len = 0;
	while (state->pos < state->source.len) {
		char ch = state->source.str[state->pos];
		++state->pos;
		++line->len;
		if (ch == '\r' || ch == '\n') { ++state->pos; break; }
	}

	// Null-terminate
	((char*)line->str)[line->len] = '\0';
	return true;
}

static bool
next_word(parse_state_t* state, str_t* word) {
	// Scan for beginning
	while (true) {
		if (state->pos >= state->source.len) { return false; }

		char ch = state->source.str[state->pos];
		if (ch == ' ' || ch == '\t') {
			++state->pos;
			continue;
		}

		word->str = state->source.str + state->pos;
		word->len = 0;
		break;
	}

	// Scan for key end
	while (true) {
		if (state->pos >= state->source.len) { return false; }

		char ch = state->source.str[state->pos];
		if (ch == '=' || ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') {
			break;
		}

		++state->pos;
		++word->len;
	}

	return true;
}

static bool
next_kv(parse_state_t* state, str_t* key, str_t* value) {
	if (state->pos >= state->source.len) { return false; }

	if (!next_word(state, key)) { return false; }

	// Scan for '='
	while (true) {
		if (state->pos >= state->source.len) { return false; }

		char ch = state->source.str[state->pos];
		++state->pos;
		if (ch == '=') { break; }
	}

	if (!next_word(state, value)) { return false; }

	++state->pos;
	// Null-terminate
	((char*)key->str)[key->len] = '\0';
	((char*)value->str)[value->len] = '\0';
	return true;
}

static char*
read_file(const char* path) {
	FILE* file = fopen(path, "rb");
	if (file == NULL) { perror("Could not open file"); return NULL; }
	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	char* content = malloc(size + 1);
	content[size] = '\0';
	fseek(file, 0, SEEK_SET);
	fread(content, 1, size, file);
	fclose(file);

	return content;
}

static inline size_t
uniform_size(CF_UniformType type) {
	switch (type) {
		case CF_UNIFORM_TYPE_INT : return sizeof(int) * 1;
		case CF_UNIFORM_TYPE_INT2: return sizeof(int) * 2;
		case CF_UNIFORM_TYPE_INT4: return sizeof(int) * 4;
		case CF_UNIFORM_TYPE_FLOAT : return sizeof(float) * 1;
		case CF_UNIFORM_TYPE_FLOAT2: return sizeof(float) * 2;
		case CF_UNIFORM_TYPE_FLOAT3: return sizeof(float) * 3;
		case CF_UNIFORM_TYPE_FLOAT4: return sizeof(float) * 4;
		default: return 0;
	}
}

static void
reload_shader(const char* source) {
	printf("Reloading shader\n");
	CF_Shader shader = cf_make_draw_shader_from_source(source);
	if (shader.id == 0) { return; }

	if (current_shader.id != 0) {
		cf_destroy_shader(current_shader);
	}

	current_shader = shader;

	parse_state_t state = { .source = { .str = source, .len = strlen(source) }};
	str_t line;

	htbl shader_param_t* tmp = current_params;
	current_params = previous_params;
	previous_params = tmp;
	hclear(current_params);
	while (next_line(&state, &line)) {
		const char* decorator_pos = byteshift_memmem(
			line.str, line.len,
			DECORATOR_PREFIX, DECORATOR_LEN
		);
		if (decorator_pos == NULL) {
			continue;
		}

		parse_state_t decorator_state = {
			.source = {
				.str = decorator_pos + DECORATOR_LEN,
				.len = line.len - (decorator_pos - line.str) - DECORATOR_LEN,
			},
		};

		shader_param_t param = { .step = 1.0f };

		str_t key, value;
		while (next_kv(&decorator_state, &key, &value)) {
			if (strcmp(key.str, "name") == 0) {
				param.name = value.str;
			} else if (strcmp(key.str, "type") == 0) {
				if (strcmp(value.str, "int") == 0) {
					param.type = CF_UNIFORM_TYPE_INT;
				} else if (strcmp(value.str, "int2") == 0) {
					param.type = CF_UNIFORM_TYPE_INT2;
				} else if (strcmp(value.str, "int4") == 0) {
					param.type = CF_UNIFORM_TYPE_INT4;
				} else if (strcmp(value.str, "float") == 0) {
					param.type = CF_UNIFORM_TYPE_FLOAT;
				} else if (strcmp(value.str, "float2") == 0) {
					param.type = CF_UNIFORM_TYPE_FLOAT2;
				} else if (strcmp(value.str, "float3") == 0) {
					param.type = CF_UNIFORM_TYPE_FLOAT3;
				} else if (strcmp(value.str, "float4") == 0) {
					param.type = CF_UNIFORM_TYPE_FLOAT4;
				} else if (strcmp(value.str, "color3") == 0) {
					param.edit_as_color = true;
					param.type = CF_UNIFORM_TYPE_FLOAT3;
				} else if (
					strcmp(value.str, "color") == 0
					||
					strcmp(value.str, "color4") == 0
				) {
					param.edit_as_color = true;
					param.type = CF_UNIFORM_TYPE_FLOAT4;
				}
			} else if (strcmp(key.str, "source") == 0) {
				if (strcmp(value.str, "ui") == 0) {
					param.source = DATA_SOURCE_UI;
				} else if (strcmp(value.str, "time") == 0) {
					param.source = DATA_SOURCE_TIME;
				} else if (strcmp(value.str, "delta_time") == 0) {
					param.source = DATA_SOURCE_DELTA_TIME;
				} else if (strcmp(value.str, "screen.w") == 0) {
					param.source = DATA_SOURCE_SCREEN_W;
				} else if (strcmp(value.str, "screen.h") == 0) {
					param.source = DATA_SOURCE_SCREEN_H;
				}
			} else if (
				strcmp(key.str, "default") == 0
				||
				strcmp(key.str, "default.u") == 0
				||
				strcmp(key.str, "default.x") == 0
				||
				strcmp(key.str, "default.r") == 0
				||
				strcmp(key.str, "default.s") == 0
			) {
				param.data.float_value[0] = strtof(value.str, NULL);
			} else if (
				strcmp(key.str, "default.v") == 0
				||
				strcmp(key.str, "default.y") == 0
				||
				strcmp(key.str, "default.g") == 0
				||
				strcmp(key.str, "default.t") == 0
			) {
				param.data.float_value[1] = strtof(value.str, NULL);
			} else if (
				strcmp(key.str, "default.z") == 0
				||
				strcmp(key.str, "default.b") == 0
				||
				strcmp(key.str, "default.p") == 0
			) {
				param.data.float_value[2] = strtof(value.str, NULL);
			} else if (
				strcmp(key.str, "default.w") == 0
				||
				strcmp(key.str, "default.a") == 0
				||
				strcmp(key.str, "default.q") == 0
			) {
				param.data.float_value[3] = strtof(value.str, NULL);
			} else if (strcmp(key.str, "target") == 0) {
				if (strcmp(value.str, "uniform") == 0) {
					param.target = PARAM_TARGET_UNIFORM;
				} else if (
					strcmp(value.str, "attribute.x") == 0
					||
					strcmp(value.str, "attribute.r") == 0
					||
					strcmp(value.str, "attribute.s") == 0
					||
					strcmp(value.str, "attribute.u") == 0
					||
					strcmp(value.str, "attribute") == 0
				) {
					param.target = PARAM_TARGET_ATTR_X;
				} else if (
					strcmp(value.str, "attribute.y") == 0
					||
					strcmp(value.str, "attribute.g") == 0
					||
					strcmp(value.str, "attribute.t") == 0
					||
					strcmp(value.str, "attribute.v") == 0
				) {
					param.target = PARAM_TARGET_ATTR_Y;
				} else if (
					strcmp(value.str, "attribute.z") == 0
					||
					strcmp(value.str, "attribute.b") == 0
					||
					strcmp(value.str, "attribute.p") == 0
				) {
					param.target = PARAM_TARGET_ATTR_Z;
				} else if (
					strcmp(value.str, "attribute.w") == 0
					||
					strcmp(value.str, "attribute.a") == 0
					||
					strcmp(value.str, "attribute.q") == 0
				) {
					param.target = PARAM_TARGET_ATTR_W;
				}
			} else if (strcmp(key.str, "min") == 0) {
				param.min = strtof(value.str, NULL);
			} else if (strcmp(key.str, "max") == 0) {
				param.max = strtof(value.str, NULL);
			} else if (strcmp(key.str, "step") == 0) {
				param.step = strtof(value.str, NULL);
			} else {
				fprintf(stderr, "Unknown attribute: %s\n", key.str);
			}
		}

		if (param.name != NULL && param.type != CF_UNIFORM_TYPE_UNKNOWN) {
			param.name = cf_sintern(param.name);

			if (previous_params != NULL) {
				shader_param_t* old_param = hfind_ptr(previous_params, param.name);
				if (old_param != NULL) {
					param.data = old_param->data;
				}
			}

			hadd(current_params, param.name, param);
		}
	}
	printf("Shader reloaded\n");
}

static void
handle_shader_changed(const char* filename, void* userdata) {
	char* file_content = read_file(filename);
	if (file_content == NULL) { return; }
	reload_shader(file_content);
	free(file_content);
}

static void
apply_shader(float* attributes) {
	cf_draw_push_shader(current_shader);
	for (int i = 0; i < hsize(current_params); ++i) {
		shader_param_t* param = &current_params[i];
		switch (param->source) {
			case DATA_SOURCE_UI: break;
			case DATA_SOURCE_TIME:
				param->data.float_value[0] = CF_SECONDS;
				break;
			case DATA_SOURCE_DELTA_TIME:
				param->data.float_value[0] = CF_DELTA_TIME;
				break;
			case DATA_SOURCE_SCREEN_W:
				if (param->type == CF_UNIFORM_TYPE_INT) {
					param->data.int_value[0] = cf_app_get_width();
				} else {
					param->data.float_value[0] = (float)cf_app_get_width();
				}
				break;
			case DATA_SOURCE_SCREEN_H:
				if (param->type == CF_UNIFORM_TYPE_INT) {
					param->data.int_value[0] = cf_app_get_height();
				} else {
					param->data.float_value[0] = (float)cf_app_get_height();
				}
				break;
		}

		switch (param->target) {
			case PARAM_TARGET_UNIFORM:
				cf_draw_set_uniform(param->name, &param->data, param->type, 1);
				break;
			case PARAM_TARGET_ATTR_X:
				memcpy(&attributes[0], &param->data, uniform_size(param->type));
				break;
			case PARAM_TARGET_ATTR_Y:
				memcpy(&attributes[1], &param->data, uniform_size(param->type));
				break;
			case PARAM_TARGET_ATTR_Z:
				memcpy(&attributes[2], &param->data, uniform_size(param->type));
				break;
			case PARAM_TARGET_ATTR_W:
				memcpy(&attributes[3], &param->data, uniform_size(param->type));
				break;
		}
	}
	cf_draw_push_vertex_attributes(attributes[0], attributes[1], attributes[2], attributes[3]);
}

static bool
str_ends_with(const char *str, const char *suffix) {
	if (!str || !suffix) { return false; }
	size_t lenstr = strlen(str);
	size_t lensuffix = strlen(suffix);
	if (lensuffix > lenstr) { return false; }

	return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}

static void
scan_for_sprites(dyna const char*** sprites) {
	cf_array_clear(*sprites);
	const char* name = cf_sintern("demo_sprite");
	cf_array_push(*sprites, name);

	const char** files = cf_fs_enumerate_directory("/");
	for (const char** itr = files; *itr != NULL; ++itr) {
		if (
			str_ends_with(*itr, ".ase")
			||
			str_ends_with(*itr, ".aseprite")
			||
			str_ends_with(*itr, ".png")
		) {
			const char* name = cf_sintern(*itr);
			cf_array_push(*sprites, name);
		}
	}
	cf_fs_free_enumerated_directory(files);

	cf_array_push(*sprites, NULL);
}

int
main(int argc, const char* argv[]) {
	if (argc != 2) {
		fprintf(stderr, "Usage: cute-shader-toy <shader.glsl>");
		return 1;
	}

	int options = CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT;
	cf_make_app("cute shader toy", 0, 0, 0, 1024, 768, options, argv[0]);
	cf_fs_mount(cf_fs_get_working_directory(), "/", true);
	cf_app_init_imgui();
	cf_set_fixed_timestep(60);
	cf_app_set_vsync(true);
	cf_clear_color(0.5f, 0.5f, 0.5f, 0.f);

	char* file_content = read_file(argv[1]);
	if (file_content == NULL) { return 1; }
	reload_shader(file_content);
	free(file_content);

	bresmon_t* monitor = bresmon_create(NULL);
	bresmon_watch(monitor, argv[1], handle_shader_changed, NULL);

	dyna const char** sprites = NULL;
	scan_for_sprites(&sprites);

	int sprite_index = 0;
	CF_Sprite sprite = cf_make_demo_sprite();
	cf_sprite_play(&sprite, "hold_down");

	float draw_scale = 5.f;
	// Make this extra big so the memcpy later is safe even with erroneous offset
	float attributes[8] = { 0 };
	int attribute_type = 0;
	const char* attribute_types[] = {
		"Color",
		"Vector",
		0
	};
	int shader_mode = SHADER_MODE_SPRITE;
	const char* shader_modes[] = {
		"Off",
		"Sprite",
		"Screen",
		0
	};

	CF_Canvas screen_canvas = cf_make_canvas(
		cf_canvas_defaults(cf_app_get_width(), cf_app_get_height())
	);

	while (cf_app_is_running()) {
		bresmon_check(monitor, false);

		cf_app_update(NULL);
		cf_sprite_update(&sprite);

		if (current_shader.id != 0 && shader_mode == SHADER_MODE_SPRITE) {
			apply_shader(attributes);
		}

		cf_draw_push();
		cf_draw_scale(draw_scale, draw_scale);
		cf_draw_sprite(&sprite);
		cf_draw_pop();

		if (current_shader.id != 0 && shader_mode == SHADER_MODE_SCREEN) {
			apply_shader(attributes);
			cf_render_to(screen_canvas, true);
			cf_draw_canvas(
				screen_canvas,
				cf_v2(0.f, 0.f),
				cf_v2(cf_app_get_width(), cf_app_get_height())
			);
		}

		if (igBegin("Shader", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
			igSeparatorText("Sprite");

			bool sprite_changed = igCombo_Str_arr("Sprite", &sprite_index, sprites, cf_array_len(sprites) - 1, -1);
			igSameLine(0.f, 10.f);
			if (igButton("Rescan", (ImVec2){ 0 })) {
				const char* old_sprite_name = sprites[sprite_index];
				scan_for_sprites(&sprites);
				bool found = false;
				for (int i = 0; i < cf_array_len(sprites); ++i) {
					if (old_sprite_name == sprites[i]) {
						sprite_index = i;
						found = true;
						break;
					}
				}

				if (!found) {
					sprite_index = 0;
					sprite_changed = true;
				}
			}

			if (sprite_changed) {
				const char* sprite_name = sprites[sprite_index];
				if (sprite_name == cf_sintern("demo_sprite")) {
					sprite = cf_make_demo_sprite();
				} else if (str_ends_with(sprite_name, ".png")) {
					sprite = cf_make_easy_sprite_from_png(sprite_name, NULL);
				} else {
					sprite = cf_make_sprite(sprite_name);
				}
				draw_scale = 1.f;
			}

			igCombo_Str_arr("Shader mode", &shader_mode, shader_modes, BCOUNT_OF(shader_modes) - 1, -1);
			igInputFloat("Scale", &draw_scale, 0.1f, 1.f, "%f", ImGuiInputTextFlags_None);

			igSeparatorText("Uniforms");
			for (int i = 0; i < hsize(current_params); ++i) {
				shader_param_t* param = &current_params[i];
				if (param->source != DATA_SOURCE_UI) { continue; }

				switch (param->type) {
					case CF_UNIFORM_TYPE_INT:
						igSliderInt(param->name, param->data.int_value, param->min, param->max, "%d", ImGuiSliderFlags_None);
						break;
					case CF_UNIFORM_TYPE_INT2:
						igSliderInt2(param->name, param->data.int_value, param->min, param->max, "%d", ImGuiSliderFlags_None);
						break;
					case CF_UNIFORM_TYPE_INT4:
						igSliderInt4(param->name, param->data.int_value, param->min, param->max, "%d", ImGuiSliderFlags_None);
						break;
					case CF_UNIFORM_TYPE_FLOAT:
						igDragFloat(param->name, param->data.float_value, param->step, param->min, param->max, "%f", ImGuiSliderFlags_None);
						break;
					case CF_UNIFORM_TYPE_FLOAT2:
						igDragFloat2(param->name, param->data.float_value, param->step, param->min, param->max, "%f", ImGuiSliderFlags_None);
						break;
					case CF_UNIFORM_TYPE_FLOAT3:
						if (param->edit_as_color) {
							igColorEdit3(param->name, param->data.float_value, ImGuiInputTextFlags_None);
						} else {
							igDragFloat3(param->name, param->data.float_value, param->step, param->min, param->max, "%f", ImGuiSliderFlags_None);
						}
						break;
					case CF_UNIFORM_TYPE_FLOAT4:
						if (param->edit_as_color) {
							igColorEdit4(param->name, param->data.float_value, ImGuiInputTextFlags_None);
						} else {
							igDragFloat4(param->name, param->data.float_value, param->step, param->min, param->max, "%f", ImGuiSliderFlags_None);
						}
						break;
					default:
						break;
				}
			}

			igSeparatorText("Vertex attribute");
			igCombo_Str_arr("Attribute type", &attribute_type, attribute_types, BCOUNT_OF(attribute_types) - 1, -1);

			if (attribute_type == 0) {
				igColorEdit4("Color", attributes, ImGuiColorEditFlags_None);
			} else {
				igInputFloat4("Vector", attributes, "%f", ImGuiInputTextFlags_None);
			}
		}
		igEnd();

		cf_app_draw_onto_screen(true);
	}

	cf_destroy_app();
	bresmon_destroy(monitor);
	hfree(previous_params);
	hfree(current_params);
	afree(sprites);

	return 0;
}

#define BLIB_IMPLEMENTATION
#include <bresmon.h>
