static const char TERRAIN_VS[] = SHADER_DEFINE(TERRAIN_HS_VECS) STRINGIFY(
// properties constant over the entire terrain: x - heightmap size,
// y - height scale
uniform vec2 constParams;
// patch-specific properties: xy - uv bias, z - scale
uniform vec3 patchParams;

// we're using vec4 because not all hardware will align this correctly
uniform vec4 heightSamples[TERRAIN_HS_VECS];

varying float fogFactor;
varying float height;

float get_height(vec2 st) {
	// flat array index
	float index = st.t * 272.0 + st.s * 16.0;	// 16 * 17 = 272
	// vector array index
	float v = index * 0.25;
	// component index
	float c = fract(v);
	v = floor(v);
	c *= 4.0;
	return heightSamples[int(v)][int(c)];
}

void main() {
	// calculate texture coordinates - offset and bias
	gl_TexCoord[0] = vec4(patchParams.z * gl_MultiTexCoord0.xy + patchParams.xy,
		gl_MultiTexCoord0.zw);

	// calculate vertex positions
	mat4 mvmat = mat4(
		// column 1
		patchParams.z * constParams.x,
		0.0,
		0.0,
		0.0,
		// column 2
		0.0,
		constParams.y,
		0.0,
		0.0,
		// column 3
		0.0,
		0.0,
		patchParams.z * constParams.x,
		0.0,
		// column 4
		(patchParams.x - 0.5) * constParams.x,
		0.0,
		(patchParams.y - 0.5) * constParams.x,
		1.0
	);
	vec4 vert = vec4(gl_Vertex.x,
		gl_Vertex.y * get_height(gl_MultiTexCoord0.xy), gl_Vertex.zw);
	gl_Position = gl_ModelViewProjectionMatrix * mvmat * vert;

	// fog stuff
	vec3 vVertex = vec3(gl_ModelViewMatrix * mvmat * vert);
	const float LOG2 = 1.442695;
	gl_FogFragCoord = length(vVertex);
	fogFactor = exp2(-gl_Fog.density * gl_Fog.density
		* gl_FogFragCoord * gl_FogFragCoord * LOG2);
	fogFactor = clamp(fogFactor, 0.0, 1.0);

	// height - for noise calculations
	height = vert.y;
}
);
