static const char FOOTMOBILE_VS[] = STRINGIFY(

void main() {
	// gl_MultiTexCoord1 holds the sprite coordinates (xyz)
	// gl_MultiTexCoord2 holds the s offset (x), t offset (y) and yaw (z)
	gl_TexCoord[0] = gl_MultiTexCoord0 + gl_MultiTexCoord2;
	// soldier's look direction
	vec3 dir = vec3(cos(gl_MultiTexCoord2.z), 0.0, -sin(gl_MultiTexCoord2.z));
	float d = dot(dir, vec3(
		gl_ModelViewMatrix[0][0],
		gl_ModelViewMatrix[1][0],
		gl_ModelViewMatrix[2][0]));
	// sneaky little hack to get rid of near-zero values
	d += 1.0 - step(0.01, abs(d));
	mat4 mat = mat4(
		// column 1
		2.0 * sign(d),	// orient the soldier accordingly
		0.0,
		0.0,
		0.0,
		// column 2
		0.0,
		2.0,
		0.0,
		0.0,
		// column 3
		0.0,
		0.0,
		2.0,
		0.0,
		gl_ModelViewMatrix * vec4(gl_MultiTexCoord1.xyz, 1.0)
	);
	gl_Position = gl_ProjectionMatrix * mat * gl_Vertex;
}
);

