#version 430 core

// VBO-ból érkező változók
layout (location = 0 ) in vec3 vs_in_pos;

// a pipeline-ban tovább adandó értékek
out vec3 vs_out_pos;

// camera + transform modules
#include "../Modules/Camera/Camera_uniforms.glsl"
#include "../Modules/Camera/Camera.glsl"
#include "../Modules/Transform/Transform_uniforms.glsl"
#include "../Modules/Transform/Transform.glsl"

void main()
{
	gl_Position = CameraViewProj(Transform(vec4(vs_in_pos, 1))).xyww;	// [x,y,w,w] => homogén osztás után [x/w, y/w, 1]

	vs_out_pos = vs_in_pos;
}
