#pragma once

#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include <GL/glew.h>

#include "Headers/Types.h"

class ShaderManager {
public:
    ~ShaderManager();

    GLuint Load(const std::string& name, const std::vector<ShaderStage>& stages);
    GLuint Load(const std::string& name, const std::vector<ShaderStage>& stages,
                const std::vector<std::string>& defines);
    GLuint Get(const std::string& name) const;
    void   ReloadAll();
    void   DeleteAll();

private:
    struct ProgramEntry {
        GLuint                   id;
        std::vector<ShaderStage> stages;
        std::vector<std::string> defines;
    };
    std::map<std::string, ProgramEntry> m_programs;
};
