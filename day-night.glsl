// Based on https://www.reddit.com/r/godot/comments/bfdb6o/my_version_of_a_dynamic_2d_daynight_cycle/
// By u/Obiwahn89
const float ra[] = float[](0.661133,-0.450579,-0.102644,0.003019,-0.017206,-0.010087);
const float rb[] = float[](-0.228901,-0.139233,0.029436,0.047411,0.007989);
const float ga[] = float[](0.626849,-0.437814,-0.050352,-0.009737,-0.028222,0.002317);
const float gb[] = float[](-0.279056,-0.106992,0.066646,0.032230,-0.001321);
const float ba[] = float[](0.725802,-0.268970,-0.035903,-0.006988,-0.021679,-0.002546);
const float bb[] = float[](-0.175541,-0.079950,0.034039,0.021785,-0.000069);
const float w[]  = float[](0.258071,0.264981,0.258893);

vec4 shader(vec4 color, vec2 pos, vec2 screen_uv, vec4 params) {
	// @param name=time type=float target=attribute.x default.r=0.0 min=0 max=24 step=0.1
    float time = params.x;
	vec4 mod_color = vec4(0);
	mod_color.a = 1.0;
	for (int i = 0; i < ra.length(); ++i) {
		mod_color.r += ra[i]*cos(i*time*w[0]);
		mod_color.g += ga[i]*cos(i*time*w[1]);
		mod_color.b += ba[i]*cos(i*time*w[2]);
	}
	for (int i = 1; i < rb.length(); ++i) {
        mod_color.r += rb[i-1]*sin(i*time*w[0]);
        mod_color.g += gb[i-1]*sin(i*time*w[1]);
        mod_color.b += bb[i-1]*sin(i*time*w[2]);
	}

    mod_color = min(mod_color, vec4(1));
	return color * mod_color;
}
