#include "ProgramBuilder.h"
#include "GLUtils.hpp"
#include <SDL3/SDL_log.h>

ProgramBuilder::ProgramBuilder(const GLuint _programID) : programID(_programID)
{
	if (programID == 0)
	{
		SDL_LogMessage(SDL_LOG_CATEGORY_ERROR,
						SDL_LOG_PRIORITY_ERROR,
						"Program needs to be inited before loading!");
		return;
	}
}

ProgramBuilder::~ProgramBuilder()
{

}

ProgramBuilder& ProgramBuilder::ShaderStage( const GLenum shaderType, const std::filesystem::path& filename)
{
    AttachShader( programID, shaderType, filename );
    return *this;
}

ProgramBuilder& ProgramBuilder::ShaderStageWithDefines(const GLenum shaderType,
                                                       const std::filesystem::path& filename,
                                                       const std::vector<std::string>& defines)
{
    AttachShaderWithDefines(programID, shaderType, filename, defines);
    return *this;
}

void ProgramBuilder::Link()
{
    LinkProgram( programID, true );
}
