#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include <GL/glew.h>

class ProgramBuilder
{
private:
	const GLuint programID;
public:
	ProgramBuilder(GLuint);
	~ProgramBuilder();
	ProgramBuilder& ShaderStage(const GLenum, const std::filesystem::path&);
	ProgramBuilder& ShaderStageWithDefines(const GLenum, const std::filesystem::path&,
	                                       const std::vector<std::string>& defines);
	void Link();
};
