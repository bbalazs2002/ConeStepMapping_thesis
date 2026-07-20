#version 430 core

// VBO-ból érkező változók
layout (location = 0 ) in vec3 vs_in_pos;
layout (location = 1 ) in vec3 vs_in_norm;
layout (location = 2 ) in vec2 vs_in_tex;

// a pipeline-ban tovább adandó értékek
out vec3 vs_out_pos;
out vec2 vs_out_tex;
out vec3 vs_out_norm;

// camera
#include "../Modules/Camera/Camera_uniforms.glsl"
#include "../Modules/Camera/Camera.glsl"

// transform
#include "../Modules/Transform/Transform_uniforms.glsl"
#include "../Modules/Transform/Transform.glsl"

void main()
{
	vs_out_norm = normalize(vs_in_norm);
	vs_out_tex = vec2(vs_in_tex.x, 1.f - vs_in_tex.y);
	vs_out_pos = Transform(vec4(vs_in_pos, 1)).xyz;
	gl_Position = CameraViewProj(Transform(vec4( vs_in_pos, 1 )));
}
