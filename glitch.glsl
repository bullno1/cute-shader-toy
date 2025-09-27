layout (set = 3, binding = 1) uniform shd_uniforms {
	// @param name=displacement type=float
    float displacement;
};

vec4 shader(vec4 color, vec2 pos, vec2 screen_uv, vec4 params) {
	float coord_displacement = displacement / u_texture_size.y;
	if (mod(screen_uv.y * u_texture_size.y, 2.0) < 1.0) {
		return texture(u_image, smooth_uv(v_uv + vec2(coord_displacement, 0.0), u_texture_size));
	} else {
		return texture(u_image, smooth_uv(v_uv + vec2(-coord_displacement, 0.0), u_texture_size));
	}
}
