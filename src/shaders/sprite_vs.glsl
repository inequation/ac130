static const char SPRITE_VS[] = STRINGIFY(
varying float alpha;

void main() {
	gl_TexCoord[0] = gl_MultiTexCoord0;
	// gl_MultiTexCoord1 holds the sprite coordinates (xyz)
	// gl_MultiTexCoord2 holds the angle (x), alpha (y) and the scale (z)
	float c = cos(gl_MultiTexCoord2.x);
	float s = sin(gl_MultiTexCoord2.x);
	mat4 mat = mat4(
		// column 1
		gl_MultiTexCoord2.z * c,
		gl_MultiTexCoord2.z * s,
		0.0,
		0.0,
		// column 2
		gl_MultiTexCoord2.z * -s,
		gl_MultiTexCoord2.z * c,
		0.0,
		0.0,
		// column 3
		0.0,
		0.0,
		gl_MultiTexCoord2.z,
		0.0,
		gl_ModelViewMatrix * vec4(gl_MultiTexCoord1.xyz, 1.0)
	);
	gl_Position = gl_ProjectionMatrix * mat * gl_Vertex;
	alpha = gl_MultiTexCoord2.y;
}
);
