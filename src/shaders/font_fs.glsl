static const char FONT_FS[] = STRINGIFY(
uniform sampler2D fontTex;

void main() {
	gl_FragColor = vec4(texture2D(fontTex, gl_TexCoord[0].st).r);
}
);

