#version 430

layout(std430, binding = 0) buffer Positions {
	vec4 pos[];
};

// Variables going forward through the pipeline
out vec3 vs_out_color;

// External parameters of the shader
uniform mat4 viewProj;

const vec3 colors[] = vec3[](
	vec3(1, 0, 0),
	vec3(0, 1, 0),
	vec3(1, 0, 1)
);

void main()
{
	uint index = gl_VertexID + 1;
	// https://registry.khronos.org/OpenGL-Refpages/gl4/html/gl_VertexID.xhtml
	gl_Position = viewProj * pos[index];
	vs_out_color = colors[gl_VertexID % 3];
}

