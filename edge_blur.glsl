vec4 shader(vec4 color, vec2 pos, vec2 screen_uv, vec4 params) {
	// @param name=blur_size type=float2 target=attribute default.x=0.6 default.y=0.6 step=0.01
	vec2 blur_size = params.xy;
	vec4 original_color = texture(u_image, smooth_uv(v_uv, u_texture_size));
	if (original_color.a > 0.0) { return original_color; }

	vec2 px_coord = v_uv * u_texture_size;
	float x_min = clamp(px_coord.x - blur_size.x, 0.0, u_texture_size.x);
	float x_max = clamp(px_coord.x + blur_size.x, 0.0, u_texture_size.x);
	float y_min = clamp(px_coord.y - blur_size.y, 0.0, u_texture_size.y);
	float y_max = clamp(px_coord.y + blur_size.y, 0.0, u_texture_size.y);
	float num_points = 0.0;
	vec4 avg_color = vec4(0.0);
	for (float x = x_min; x <= x_max; x += 0.01) {
		for (float y = y_min; y <= y_max; y += 0.01) {
			vec4 neighbor_color = texture(u_image, vec2(x, y) / u_texture_size);
			if (neighbor_color.a > 0.0) {
				avg_color += neighbor_color;
				num_points += 1.0;
				float x_diff = x - px_coord.x;
				float y_diff = y - px_coord.y;
				float point_distance = x_diff * x_diff + y_diff * y_diff;
			}
		}
	}
	avg_color /= num_points;

	return avg_color;
}
