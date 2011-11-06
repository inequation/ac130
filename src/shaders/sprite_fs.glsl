static const char SPRITE_FS[] = STRINGIFY(
uniform sampler2D spriteTex;
varying float alpha;

void main() {
	vec4 c = texture2D(spriteTex, gl_TexCoord[0].st);
	gl_FragColor = vec4(c.r, c.r, c.r, c.a * alpha);
}
);
