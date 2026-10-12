// This file may contain AI-assisted (Claude Code) code, reviewed by the author.

struct CameraUniforms{
	mat4 viewProj;
	vec3 at;
	vec3 up;
	vec3 eye;
};

uniform CameraUniforms cameraData;