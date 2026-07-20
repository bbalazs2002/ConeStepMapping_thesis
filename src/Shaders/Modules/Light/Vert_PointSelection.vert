#version 430

out vec3 vs_out_col;

// camera
#include "../Camera/Camera_uniforms.glsl"
#include "../Camera/Camera.glsl"

// light
#define LIGHT_LIGHTS_SSBO 3
#include "Light_uniforms.glsl"

uniform int lightID = 0;

// sun
const vec3 weights[] = vec3[24](
	// top
	vec3(0,1,0),
	vec3(-.25,0,0),
	vec3(.25,0,0),
	// right
	vec3(1,0,0),
	vec3(0,.25,0),
	vec3(0,-.25,0),
	// bottom
	vec3(0,-1,0),
	vec3(.25,0,0),
	vec3(-.25,0,0),
	// left
	vec3(-1,0,0),
	vec3(0,-.25,0),
	vec3(0,.25,0),
	// top right
	vec3(.71,.71,0),
	vec3(0,.25,0),
	vec3(0,-.25,0),
	// bottom right
	vec3(.71,-.71,0),
	vec3(.25,0,0),
	vec3(0,-.25,0),
	// bottom left
	vec3(-.71,-.71,0),
	vec3(0,-.25,0),
	vec3(-.25,0,0),
	// top left
	vec3(-.71,.71,0),
	vec3(-.25,0,0),
	vec3(0,.25,0)
);

void main() {
	// calculate model base

	vec3 w = normalize(cameraData.at - cameraData.eye);
	vec3 v = normalize(cross(w, cameraData.up));
	vec3 u = normalize(cross(w, v));
	mat3 base = mat3(u,v,w);

	gl_Position = CameraViewProj(vec4(lightSources[lightID].position.xyz + .4f * base * weights[gl_VertexID], 1));

	vs_out_col = vec3(1,1,0);
}
