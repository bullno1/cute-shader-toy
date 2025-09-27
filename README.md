# cute-shader-toy

Shader-toy like tool for [Cute Framework](https://randygaul.github.io/cute_framework/).

# How to use

`bin/RelWithDebInfo/cute-shader-toy default.glsl`.

As the shader will be automatically reloaded when it is updated.

Special comments can be used to inform the program about uniforms so it can show them in the UI.

The syntax is as follow: `// @param key=value`.
The following keys are recognized.

* `name` (required): Name of the uniform.
* `type` (required): Type of the uniform, the same type names as GLSL are recognized.
  In addition, `color3` and `color4` are aliases for `float3` and `float4` accordingly but a picker is shown in the UI instead.
  `color` is also an alias for `color4`.
* `source` (optional): How is the uniform set:

  * `ui`: The value is set in the UI.
    This is the default.
  * `time`: Automatically set to [`CF_SECONDS`](https://randygaul.github.io/cute_framework/time/cf_seconds/) every frame.
  * `delta_time`: Automatically set to [`CF_DELTA_TIME`](https://randygaul.github.io/cute_framework/time/cf_delta_time/) every frame.
* `default` (optional): Default value for each uniform.

  * `default`, `default.x`, `default.r`, `default.u`, `default.s`: Set the first component.
  * `default.y`, `default.g`, `default.v`, `default.t`: Set the second component.
  * `default.z`, `default.b`, `default.p`: Set the third component.
  * `default.w`, `default.a`, `default.q`: Set the fourth component.

For an example, refer to [glitch.glsl](./glitch.glsl).
