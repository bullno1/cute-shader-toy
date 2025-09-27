layout (set = 3, binding = 1) uniform shd_uniforms {
	// @param name=tint_color type=color4
    vec4 tint_color;
};

vec4 shader(vec4 color, vec2 pos, vec2 screen_uv, vec4 params) {
	return color.rgba * tint_color.rgba;
}
