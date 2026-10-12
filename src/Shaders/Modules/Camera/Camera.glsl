// This file may contain AI-assisted (Claude Code) code, reviewed by the author.

vec4 CameraViewProj(vec4 pos) {
	return cameraData.viewProj * pos;
}