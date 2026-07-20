#version 430 core

layout (location = 0 ) in vec3 vs_in_pos;

// camera
#include "../Modules/Camera/Camera_uniforms.glsl"
#include "../Modules/Camera/Camera.glsl"

// transform
#include "../Modules/Transform/Transform_uniforms.glsl"
#include "../Modules/Transform/Transform.glsl"

void main()
{
	gl_Position = CameraViewProj(Transform(vec4(vs_in_pos, 1)));
}
