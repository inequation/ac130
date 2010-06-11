static const char COMPOSITOR_FS[] = STRINGIFY(
uniform sampler2D overlay;
uniform sampler2D frames[6];	// MUST be kept in sync with 1 + FRAME_TRACE!!!
uniform float negative;
uniform float cont;

vec4 get_view(vec2 st) {
	// add past frames together to achieve the inertia effect
	vec4 v = 0.705 * texture2D(frames[5], st)
		+ 0.105 * texture2D(frames[4], st)
		+ 0.085 * texture2D(frames[3], st)
		+ 0.065 * texture2D(frames[2], st)
		+ 0.045 * texture2D(frames[1], st)
		+ 0.025 * texture2D(frames[0], st);
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

const float blurSize = 0.8 / 1024.0;
vec4 get_overlay(vec2 st) {
	vec4 sum = vec4(0.0);
	sum += texture2D(overlay, vec2(st.x - 8.0 * blurSize, st.y)) * 0.022;
	sum += texture2D(overlay, vec2(st.x - 7.0 * blurSize, st.y)) * 0.044;
	sum += texture2D(overlay, vec2(st.x - 6.0 * blurSize, st.y)) * 0.066;
	sum += texture2D(overlay, vec2(st.x - 5.0 * blurSize, st.y)) * 0.088;
	sum += texture2D(overlay, vec2(st.x - 4.0 * blurSize, st.y)) * 0.111;
	sum += texture2D(overlay, vec2(st.x - 3.0 * blurSize, st.y)) * 0.133;
	sum += texture2D(overlay, vec2(st.x - 2.0 * blurSize, st.y)) * 0.155;
	sum += texture2D(overlay, vec2(st.x - blurSize, st.y)) * 0.177;
	sum += texture2D(overlay, vec2(st.x, st.y)) * 0.2;
	return sum;
}

void main() {
	vec4 ov = get_overlay(gl_TexCoord[0].st);
	vec4 view = get_view(gl_TexCoord[0].st);
	gl_FragColor = vec4(mix(view.rgb, ov.rgb, ov.a), 1.0);
}
);
