#include "Headers/Manager/ShaderManager.h"
#include "Headers/Types.h"
#include "Utils/ProgramBuilder.h"
#include "Utils/Log.h"

ShaderManager::~ShaderManager()
{
    DeleteAll();
}

GLuint ShaderManager::Load(const std::string& name, const std::vector<ShaderStage>& stages)
{
    GLuint prog = glCreateProgram();
    ProgramBuilder builder(prog);
    for (const auto& stage : stages)
        builder.ShaderStage(stage.type, stage.path);
    builder.Link();
    m_programs[name] = { prog, stages, {} };
    return prog;
}

GLuint ShaderManager::Load(const std::string& name, const std::vector<ShaderStage>& stages,
                           const std::vector<std::string>& defines)
{
    if (defines.empty())
        return Load(name, stages);

    GLuint prog = glCreateProgram();
    ProgramBuilder builder(prog);
    for (const auto& stage : stages)
        builder.ShaderStageWithDefines(stage.type, stage.path, defines);
    builder.Link();
    m_programs[name] = { prog, stages, defines };
    return prog;
}

GLuint ShaderManager::Get(const std::string& name) const
{
    auto it = m_programs.find(name);
    if (it == m_programs.end()) {
        LOG_ERROR("ShaderManager: program '", name, "' not found");
        return 0;
    }
    return it->second.id;
}

void ShaderManager::ReloadAll()
{
    // Relink each program in place — keep the same GLuint so that every
    // class holding a cached programID stays valid after a reload.
    for (auto& [name, entry] : m_programs) {
        ProgramBuilder builder(entry.id);
        if (entry.defines.empty()) {
            for (const auto& stage : entry.stages)
                builder.ShaderStage(stage.type, stage.path);
        } else {
            for (const auto& stage : entry.stages)
                builder.ShaderStageWithDefines(stage.type, stage.path, entry.defines);
        }
        builder.Link();
    }
}

void ShaderManager::DeleteAll()
{
    for (auto& [name, entry] : m_programs)
        glDeleteProgram(entry.id);
    m_programs.clear();
}
