#version 430

// Variables going forward through the pipeline
out vec3 vs_out_color;

// camera + transform modules
#include "../Modules/Camera/Camera_uniforms.glsl"
#include "../Modules/Camera/Camera.glsl"
#include "../Modules/Transform/Transform_uniforms.glsl"
#include "../Modules/Transform/Transform.glsl"

const vec4 positions[6] = vec4[6](
	// 1. segment (X)
	vec4( 0,  0, 0, 1),
	vec4( 1,  0, 0, 1),
	// 2. segment (Y)
	vec4( 0,  0, 0, 1),
	vec4( 0,  1, 0, 1),
	// 3. segment (Z)
	vec4( 0,  0, 0, 1),
	vec4( 0,  0, 1, 1)
);

const vec3 colors[6] = vec3[6](
	vec3(1, 0, 0),
	vec3(1, 0, 0),
	vec3(0, 1, 0),
	vec3(0, 1, 0),
	vec3(0, 0, 1),
	vec3(0, 0, 1)
);

void main()
{
	gl_Position = CameraViewProj(Transform(positions[gl_VertexID]));
	vs_out_color = colors[gl_VertexID];
}
