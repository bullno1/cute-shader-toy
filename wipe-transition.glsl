vec4 shader(vec4 color, vec2 pos, vec2 screen_uv, vec4 params) {
	// @param name=left_to_right type=float target=attribute.x default=0.0 min=0.0 max=1.0 step=1.0
	float left_to_right = params.x;
	// @param name=slope type=float target=attribute.y default=0.0 min=0.0 max=1.5 step=0.01
	float slope = params.y;
	// @param name=t type=float target=attribute.z default=0.0 min=0.0 max=1.0 step=0.01
	float t = params.z;

	if (left_to_right == 0.0) { t = 1.0 - t; }
	float scaled_time = mix(-slope, 1.0 + slope, t);
	float threshold = scaled_time + mix(slope, -slope, screen_uv.y);

	if (left_to_right == 1.0) {
		if (screen_uv.x < threshold) { color.a = 0.0; }
	} else {
		if (screen_uv.x > threshold) { color.a = 0.0; }
	}

    return color;
}
