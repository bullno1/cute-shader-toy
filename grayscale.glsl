layout (set = 3, binding = 1) uniform shd_uniforms {
	// @param name=color_factor type=float default=0.5 max=1.0 step=0.01
	float color_factor;
};

vec4 shader(vec4 color, vec2 pos, vec2 screen_uv, vec4 params) {
	float gray = 0.21 * color.r + 0.71 * color.g + 0.07 * color.b;
	return vec4(color.rgb * (1.0 - color_factor) + (gray * color_factor), color.a);
}
