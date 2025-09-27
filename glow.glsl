layout (set = 3, binding = 1) uniform shd_uniforms {
	// @param name=glow_color type=color default.r=1.0 default.g=0.85 default.b=0.20 default.a=1.0
	vec4 glow_color;
	// @param name=glow_size type=float2 default.x=0.6 default.y=0.6
	vec2 glow_size;
};

vec4 shader(vec4 color, vec2 pos, vec2 screen_uv, vec4 params) {
	vec4 original_color = texture(u_image, smooth_uv(v_uv, u_texture_size));
	if (original_color.a > 0.0) { return original_color; }

	vec4 new_color = vec4(glow_color.rgb, 0.0);
	vec2 px_coord = v_uv * u_texture_size;
	float max_distance = glow_size.x * glow_size.x + glow_size.y * glow_size.y;
	float distance = max_distance;
	float x_min = clamp(px_coord.x - glow_size.x, 0.0, u_texture_size.x);
	float x_max = clamp(px_coord.x + glow_size.x, 0.0, u_texture_size.x);
	float y_min = clamp(px_coord.y - glow_size.y, 0.0, u_texture_size.y);
	float y_max = clamp(px_coord.y + glow_size.y, 0.0, u_texture_size.y);
	for (float x = x_min; x <= x_max; x += 0.01) {
		for (float y = y_min; y <= y_max; y += 0.01) {
			vec4 neighbor_color = texture(u_image, smooth_uv(vec2(x, y) / u_texture_size, u_texture_size));
			if (neighbor_color.a > 0.0) {
				float x_diff = x - px_coord.x;
				float y_diff = y - px_coord.y;
				float point_distance = x_diff * x_diff + y_diff * y_diff;
				if (point_distance < distance) {
					distance = point_distance;
					original_color = neighbor_color;
				}
			}
		}
	}
	new_color.a = (1.0 - sqrt(distance / max_distance));

	return new_color;
}
