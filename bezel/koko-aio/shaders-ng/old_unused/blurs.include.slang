vec4 blur9(sampler2D image, vec2 uv, vec2 resolution, vec2 direction, float lod) {
	vec4 color = vec4(0.0);
	vec2 off1 = vec2(1.3846153846) * direction;
	vec2 off2 = vec2(3.2307692308) * direction;
	color += textureLod(image, uv, lod) * 0.2270270270;
	color += textureLod(image, uv + (off1 / resolution), lod) * 0.3162162162;
	color += textureLod(image, uv - (off1 / resolution), lod) * 0.3162162162;
	color += textureLod(image, uv + (off2 / resolution), lod) * 0.0702702703;
	color += textureLod(image, uv - (off2 / resolution), lod) * 0.0702702703;
	return color;
}

