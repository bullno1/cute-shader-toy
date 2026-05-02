#include "blend.shd"

layout (set = 3, binding = 1) uniform shd_uniforms {
	// @param name=hue type=float min=0.0 max=1.0 step=0.01
    float hue;
};

vec4 shader(vec4 color, ShaderParams params) {
	vec3 hsv = rgb_to_hsv(color.rgb);
	vec4 new_olor = vec4(hsv_to_rgb(vec3(hue, 0.0, 0.0) + hsv), color.a);
	return new_olor;
}
