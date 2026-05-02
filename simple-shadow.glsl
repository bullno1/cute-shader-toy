// Inspired by: https://bsky.app/profile/mariobrothblog.bsky.social/post/3mksjoaqaok2b
vec4 shader(vec4 color, ShaderParams params) {
	// @param name=shadow_offset type=float2 step=1 target=attribute.x
	vec2 shadow_offset = params.attributes.xy;

	// @param name=shadow_alpha type=float default=0.5 step=0.01 min=0.0 max=1.0 target=attribute.a
	float shadow_alpha = params.attributes.a;

	vec2 uv_offset = shadow_offset / u_texture_size;
	vec2 shadow_uv = clamp(uv_offset + params.uv, params.uv_min, params.uv_max);
	vec4 shadow_color = texture(u_image, smooth_uv(shadow_uv, u_texture_size));

    return vec4(mix(color.rgb, vec3(0.0), shadow_color.a * shadow_alpha), color.a);
}
