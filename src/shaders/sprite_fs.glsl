static const char SPRITE_FS[] = STRINGIFY(
uniform sampler2D spriteTex;
varying vec4 colour;

void main() {
	gl_FragColor = texture2D(spriteTex, gl_TexCoord[0].st) * colour;
}
);
