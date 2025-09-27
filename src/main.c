#include <bresmon.h>
#include <cute.h>
#include <cimgui.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
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

static CF_Shader current_shader = { 0 };

static bool
next_line(parse_state_t* state, str_t* line) {
	if (state->pos >= state->source.len) { return false; }

	line->str = state->source.str + state->pos;
	line->len = 0;
	while (state->pos < state->source.len) {
		++state->pos;
		++line->len;
		char ch = state->source.str[state->pos] == '\n';
		if (ch == '\r' || ch == '\n') { break; }
	}

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

	return next_word(state, value);
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

static void
reload_shader(const char* source) {
	CF_Shader shader = cf_make_draw_shader_from_source(source);
	if (shader.id == 0) { return; }

	if (current_shader.id != 0) {
		cf_destroy_shader(current_shader);
	}

	current_shader = shader;

	parse_state_t state = { .source = { .str = source, .len = strlen(source) }};
	str_t line;
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
		str_t key, value;
		while (next_kv(&decorator_state, &key, &value)) {
			printf(
				"%.*s = %.*s\n",
				(int)key.len, key.str,
				(int)value.len, value.str
			);
		}
	}
}

static void
handle_shader_changed(const char* filename, void* userdata) {
	char* file_content = read_file(filename);
	if (file_content == NULL) { return; }
	reload_shader(file_content);
	free(file_content);
}

int
main(int argc, const char* argv[]) {
	if (argc != 2) {
		fprintf(stderr, "Usage: cute-shader-toy <shader.glsl>");
		return 1;
	}

	int options = CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT;
	cf_make_app("cute shader toy", 0, 0, 0, 1280, 720, options, argv[0]);
	cf_app_init_imgui();
	cf_set_fixed_timestep(60);
	cf_app_set_vsync(true);
	cf_clear_color(0.5f, 0.5f, 0.5f, 1.f);

	char* file_content = read_file(argv[1]);
	if (file_content == NULL) { return 1; }
	reload_shader(file_content);
	free(file_content);

	bresmon_t* monitor = bresmon_create(NULL);
	bresmon_watch(monitor, argv[1], handle_shader_changed, NULL);

	CF_Sprite sprite = cf_make_demo_sprite();

	while (cf_app_is_running()) {
		cf_app_update(NULL);
		cf_sprite_update(&sprite);

		if (current_shader.id) {  cf_draw_push_shader(current_shader); }
		cf_draw_sprite(&sprite);

		if (igBegin("Shader params", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
		}
		igEnd();

		cf_app_draw_onto_screen(true);
	}

	cf_destroy_app();
	bresmon_destroy(monitor);

	return 0;
}

#define BLIB_IMPLEMENTATION
#include <bresmon.h>
