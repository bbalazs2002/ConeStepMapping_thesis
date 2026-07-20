#version 430

out vec3 vs_out_col;

// camera
#include "../Camera/Camera_uniforms.glsl"
#include "../Camera/Camera.glsl"

// light
#define LIGHT_LIGHTS_SSBO 3
#include "Light_uniforms.glsl"

uniform int isInner = 0;
uniform int lightID = 0;

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

	int isCenter = max(0, 1 - gl_VertexID);		// 1 if it's the first vertex; 0 otherwise

	float n = 20.;
	float delta = 2. * 3.14159265 / n;
	float i = float(gl_VertexID - 1);

	float angle = isInner * lightSources[lightID].flags_angle_plane.z + (1 - isInner) * lightSources[lightID].flags_angle_plane.y;
	float dist = length(lightSources[lightID].position.xyz - cameraData.eye);
	float m = dist * lightSources[lightID].flags_angle_plane.z / 10.;	// height of the cone (scaled by distance from the camera and outer angle size)
	float r = m * tan(angle);											// radius of the base circle
	vec3 P = vec3(m, r * cos(i * delta), r * sin(i * delta));			// point on the base circle contour

	gl_Position = CameraViewProj(vec4(lightSources[lightID].position.xyz + base * (float(isCenter) * vec3(0) + float(1 - isCenter) * P), 1));

	vs_out_col = float(1 - isInner) * vec3(1,0,0) + float(isInner) * (float(isCenter) * vec3(1,1,0) + float(1 - isCenter) * vec3(1));
}
