static const char TERRAIN_FS[] = STRINGIFY(
uniform sampler2D terTex;
varying float fogFactor;
varying float height;

void main() {
	gl_FragColor = mix(gl_Fog.color,
		texture2D(terTex, gl_TexCoord[0].st), fogFactor);
}
);
