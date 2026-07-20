#version 430

out vec3 vs_out_col;

// camera
#include "../Camera/Camera_uniforms.glsl"
#include "../Camera/Camera.glsl"

// light
#define LIGHT_LIGHTS_SSBO 3
#include "Light_uniforms.glsl"

uniform int lightID = 0;

// colors
const vec3 colors[] = vec3[](
	// tip
	vec3(1,0,0),
	vec3(0,1,0),
	vec3(0,0,1),
	// tip back
	vec3(1,1,0),
	// handle back
	vec3(1,0,1),
	// handle side1
	vec3(0,1,0),
	vec3(0,1,0),
	// handle side2
	vec3(1,0,0),
	vec3(1,0,0),
	// handle side3
	vec3(0,0,1),
	vec3(0,0,1)
);

// arrow
const vec3 weights[] = vec3[](
	// tip
	vec3(10,0,0),
	vec3(7,0,2),
	vec3(7,-1.73,-1),
	//
	vec3(10,0,0),
	vec3(7,1.73,-1),
	vec3(7,0,2),
	//
	vec3(10,0,0),
	vec3(7,-1.73,-1),
	vec3(7,1.73,-1),
	// tip back
	vec3(7,0,2),
	vec3(7,1.73,-1),
	vec3(7,-1.73,-1),
	// handle back
	vec3(0,0,1),
	vec3(0,.87,-.5),
	vec3(0,-.87,-.5),
	// handle side1
	vec3(0,0,1),
	vec3(7,.87,-.5),
	vec3(0,.87,-.5),
	//
	vec3(7,0,1),
	vec3(7,.87,-.5),
	vec3(0,0,1),
	// handle side2
	vec3(0,0,1),
	vec3(0,-.87,-.5),
	vec3(7,-.87,-.5),
	//
	vec3(7,0,1),
	vec3(0,0,1),
	vec3(7,-.87,-.5),
	// handle side3
	vec3(0,.87,-.5),
	vec3(7,.87,-.5),
	vec3(0,-.87,-.5),
	//
	vec3(7,.87,-.5),
	vec3(7,-.87,-.5),
	vec3(0,-.87,-.5)
);

void main() {
	// calculate model base
	vec3 u = normalize(lightSources[lightID].direction.xyz);
	vec3 v = vec3(0);
	float d = dot(u, vec3(0,1,0));
	if (-.8 < d && d < .8) {	// not near to 180° or -180°
		v = normalize(cross(u, vec3(0,1,0)));
	} else {
		v = normalize(cross(u, vec3(1,0,0)));
	}
	vec3 w = normalize(cross(u, v));
	mat3 base = mat3(u,v,w);

	gl_Position = CameraViewProj(vec4(cameraData.at + .1 * base * weights[gl_VertexID], 1));

	vs_out_col = colors[int(floor(float(gl_VertexID) / 3.f))];
}
