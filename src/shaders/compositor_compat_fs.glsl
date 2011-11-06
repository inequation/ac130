static const char COMPOSITOR_COMPAT_FS[] = STRINGIFY(
uniform sampler2D overlay;
uniform sampler2D frames[6];	// MUST be kept in sync with 1 + FRAME_TRACE!!!
uniform float negative;
uniform float cont;

vec4 get_view(vec2 st) {
	vec4 v = texture2D(frames[5], st);
	// enhance the contrast
	vec3 c = mix(v.rgb * 0.4,
		vec3(1.0) - 0.4 * (vec3(1.0) - v.rgb),
		smoothstep(0.5, 0.75, v.r));
	// mix the normal and contrast-enhanced versions
	vec3 p = mix(v.rgb, c, cont);
	// find the negative
	vec3 n = vec3(1.0) - p.rgb;
	// mix the positive and negatives
	return vec4(mix(p, n, negative), 1.0);
}

vec4 get_overlay(vec2 st) {
	return texture2D(overlay, vec2(st.x, st.y));
}

void main() {
	vec4 ov = get_overlay(gl_TexCoord[0].st);
	vec4 view = get_view(gl_TexCoord[0].st);
	gl_FragColor = vec4(mix(view.rgb, ov.rgb, ov.a), 1.0);
}
);

