#define PI 3.14159265359

layout (set = 3, binding = 1) uniform shd_uniforms {
	// @param name=max_displacement type=float default=1.0
	float max_displacement;
	// @param name=normal_period type=float default=0.5
	float normal_period;
	// @param name=glitch_period type=float default=0.5
	float glitch_period;
	// @param name=time type=float source=time
	float time;
	// @param name=screen_height type=int source=screen.h
	int screen_height;
};

vec4 shader(vec4 color, vec2 pos, vec2 screen_uv, vec4 params) {
	float time_offset = mod(time, normal_period + glitch_period);
	if (time_offset < glitch_period) {
		float displacement = max_displacement * sin(time_offset / glitch_period * PI * 2);
		float coord_displacement = displacement / u_texture_size.y;
		if (mod(screen_uv.y * float(screen_height), 2.0) < 1.0) {
			return texture(u_image, smooth_uv(v_uv + vec2(coord_displacement, 0.0), u_texture_size));
		} else {
			return texture(u_image, smooth_uv(v_uv + vec2(-coord_displacement, 0.0), u_texture_size));
		}
	} else {
		return texture(u_image, smooth_uv(v_uv, u_texture_size));
	}
}
