#include "GLUtils.hpp"

#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <regex>
#include <set>

#include <SDL3_image/SDL_image.h>

#include "Log.h"

#ifndef PROJECT_ROOT
#pragma message(__FILE__ "("  ": warning: PROJECT_ROOT is not defined! Fallback route: ./")
#define PROJECT_ROOT "./"
#endif

///////////////////////////
// --- SHADER LOADER --- //
///////////////////////////
static void loadShaderCode(std::string& shaderCode, const std::filesystem::path& fileAbsName) {

    // Open the stream starting at the end of the file (ate) to determine size immediately
    std::ifstream shaderStream(fileAbsName, std::ios::in | std::ios::binary | std::ios::ate);

    if (!shaderStream.is_open()) {
        LOG_ERROR("[GLUtils] Error while opening shader: ", fileAbsName.generic_string());
        return;
    }

    // Get the file size and reserve memory in the string to avoid multiple reallocations
    std::streamsize size = shaderStream.tellg();
    shaderStream.seekg(0, std::ios::beg);

    shaderCode.clear();
    if (size > 0) {
        shaderCode.resize(static_cast<size_t>(size));
        if (!shaderStream.read(shaderCode.data(), size)) {
            LOG_ERROR("[GLUtils] Error while reading shader: ", fileAbsName.generic_string());
            shaderCode.clear();
        }
    }

    shaderStream.close();
}

// Recursive shader preprocessor to handle custom #include directives
static void preprocessShaderCodeRecursive(std::string& sourceCode, const std::filesystem::path& currentAbsPath, std::set<std::string>& includedFiles) {
    // Regex is static to avoid recompiling it for every recursive call
    static const std::regex includeRegex(R"(#include\s*\"([^\"]+)\")");
    std::smatch match;

    std::ostringstream processedCode;
    std::string::const_iterator searchStart(sourceCode.cbegin());
    int currentLine = 1;

    while (std::regex_search(searchStart, sourceCode.cend(), match, includeRegex)) {
        // Append text before the #include directive
        std::string segment(searchStart, match[0].first);
        processedCode << segment;
        currentLine += std::count(segment.begin(), segment.end(), '\n');

        // Resolve relative path
        std::filesystem::path includePath = currentAbsPath.parent_path() / match[1].str();

        std::error_code ec;
        std::filesystem::path canonicalPath = std::filesystem::weakly_canonical(includePath, ec);
        std::string uniqueKey = canonicalPath.generic_string();

        // Prevent circular or duplicate includes
        if (!ec && includedFiles.find(uniqueKey) == includedFiles.end()) {
            includedFiles.insert(uniqueKey);
            LOG("\t - Shader preprocessor include: ", includePath.generic_string());

            std::string includedCode;
            loadShaderCode(includedCode, includePath);

            if (!includedCode.empty()) {
                preprocessShaderCodeRecursive(includedCode, includePath, includedFiles);

                // Use #line to ensure shader compiler errors point to the correct file/line
                processedCode << "\n#line 1\n" << includedCode << "\n#line " << (currentLine + 1) << "\n";
            }
        }
        else {
            processedCode << "// Skipped duplicate/missing include: " << match[1].str() << "\n";
        }

        searchStart = match[0].second;
    }

    processedCode << std::string(searchStart, sourceCode.cend());
    sourceCode = processedCode.str();
}

static void preprocessShaderCode(std::string& shaderCode, const std::filesystem::path& includerAbsPath) {
    std::set<std::string> includedFiles;
    std::error_code ec;
    includedFiles.insert(std::filesystem::weakly_canonical(includerAbsPath, ec).generic_string());

    // Basic protection: OpenGL requires #version to be the very first line.
    // If found, we should ideally keep it at the top.
    preprocessShaderCodeRecursive(shaderCode, includerAbsPath, includedFiles);
}

GLuint AttachShader(const GLuint programID, GLenum shaderType, const std::filesystem::path& fileName) {

    std::filesystem::path fullPath = std::filesystem::path(PROJECT_ROOT) / fileName;

    LOG("[GLUtils] Loading shader: ", fullPath.generic_string());

    std::string shaderCode;
    loadShaderCode(shaderCode, fullPath.generic_string());

    preprocessShaderCode(shaderCode, fullPath);

    return AttachShaderCode(programID, shaderType, shaderCode);
}

// Reads the file, preprocesses #includes, injects defines after the #version line, then compiles.
GLuint AttachShaderWithDefines(const GLuint programID, GLenum shaderType,
                               const std::filesystem::path& fileName,
                               const std::vector<std::string>& defines)
{
    if (defines.empty())
        return AttachShader(programID, shaderType, fileName);

    std::filesystem::path fullPath = std::filesystem::path(PROJECT_ROOT) / fileName;

    LOG("[GLUtils] Loading shader (with defines): ", fullPath.generic_string());

    std::string shaderCode;
    loadShaderCode(shaderCode, fullPath.generic_string());
    preprocessShaderCode(shaderCode, fullPath);

    // Inject defines immediately after the #version line so they are visible everywhere.
    // OpenGL requires #version to be the very first token, so we must not prepend before it.
    auto newlinePos = shaderCode.find('\n');
    if (newlinePos != std::string::npos) {
        std::string defineBlock;
        for (const auto& d : defines)
            defineBlock += "#define " + d + "\n";
        shaderCode.insert(newlinePos + 1, defineBlock);
    }

    return AttachShaderCode(programID, shaderType, shaderCode);
}

GLuint AttachShaderCode(const GLuint programID, GLenum shaderType, std::string_view shaderCode) {
    if (programID == 0) {
        LOG_ERROR("[GLUtils] AttachShaderCode: Program ID is 0! Init program before loading shaders.");
        return 0;
    }

    GLuint shaderID = glCreateShader(shaderType);
    const char* sourcePointer = shaderCode.data();
    GLint sourceLength = static_cast<GLint>(shaderCode.length());

    glShaderSource(shaderID, 1, &sourcePointer, &sourceLength);
    glCompileShader(shaderID);

    GLint result = GL_FALSE;
    int infoLogLength;
    glGetShaderiv(shaderID, GL_COMPILE_STATUS, &result);
    glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &infoLogLength);

    if (result == GL_FALSE && infoLogLength > 0) {
        std::string errorMessage(infoLogLength, '\0');
        glGetShaderInfoLog(shaderID, infoLogLength, nullptr, errorMessage.data());
        LOG_ERROR("[GLUtils] glCompileShader Error: ", errorMessage);
    }

    glAttachShader(programID, shaderID);
    return shaderID;
}

void LinkProgram(const GLuint programID, bool ownShaders) {
    if (programID == 0 || SDL_GL_GetCurrentContext() == nullptr) {
        LOG_ERROR("[GLUtils] LinkProgram: Invalid program ID or no GL context!");
        return;
    }

    glLinkProgram(programID);

    GLint linkStatus = GL_FALSE;
    GLint infoLogLength = 0;
    glGetProgramiv(programID, GL_LINK_STATUS, &linkStatus);
    glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &infoLogLength);

    if (linkStatus == GL_FALSE && infoLogLength > 0) {
        std::string errorMessage(infoLogLength, '\0');
        glGetProgramInfoLog(programID, infoLogLength, nullptr, errorMessage.data());
        LOG_ERROR("[GLUtils] glLinkProgram Error: ", errorMessage);
    }

    // If the program owns the shaders, detach and delete them to save GPU memory
    if (ownShaders) {
        GLint attachedCount = 0;
        glGetProgramiv(programID, GL_ATTACHED_SHADERS, &attachedCount);

        if (attachedCount > 0) {
            std::vector<GLuint> shaders(static_cast<size_t>(attachedCount));
            glGetAttachedShaders(programID, attachedCount, nullptr, shaders.data());

            for (GLuint shader : shaders) {
                glDetachShader(programID, shader);
                glDeleteShader(shader);
            }
        }
    }
}

////////////////////////////
// --- TEXTURE LOADER --- //
////////////////////////////
static inline ImageRGBA::TexelRGBA* get_image_row( ImageRGBA& image, int rowIndex )
{
	return &image.texelData[  rowIndex * image.width ];
}

static void invert_image_RGBA(ImageRGBA& image)
{
	int height_div_2 = image.height / 2;


	for ( int index = 0; index < height_div_2; index++ )
	{
		std::uint32_t* lower_data  =reinterpret_cast<std::uint32_t*>(get_image_row( image, index) );
		std::uint32_t* higher_data =reinterpret_cast<std::uint32_t*>(get_image_row( image, image.height - 1 - index ) );

		for ( unsigned int rowIndex = 0; rowIndex < image.width; rowIndex++ )
		{
			lower_data[ rowIndex ] ^= higher_data[ rowIndex ];
			higher_data[ rowIndex ] ^= lower_data[ rowIndex ];
			lower_data[ rowIndex ] ^= higher_data[ rowIndex ];
		}
	}
}

GLsizei NumberOfMIPLevels( const ImageRGBA& image )
{
	GLsizei targetlevel = 1;
	unsigned int index = std::max( image.width, image.height );

	while (index >>= 1) ++targetlevel;

	return targetlevel;
}

[[nodiscard]] ImageRGBA ImageFromFile(const std::filesystem::path& fileName, bool needsFlip) {
    const std::filesystem::path fullPath = std::filesystem::path(PROJECT_ROOT) / fileName;
    LOG("[GLUtils] Loading texture: ", fullPath.generic_string());

    ImageRGBA img;

    // 1. Open the file using UTF-8 path
    std::string pathStr = fullPath.generic_string();
    SDL_IOStream* stream = SDL_IOFromFile(pathStr.c_str(), "rb");

    if (!stream) {
        LOG_ERROR("[GLUtils] File not found: ", pathStr);
        return img;
    }

    // 2. Load image - SDL3_image automatically detects formats like PNG, JPG, etc.
    // The second parameter 'true' tells IMG_Load_IO to close the stream for us.
    SDL_Surface* raw_surf = IMG_Load_IO(stream, true);

    if (!raw_surf) {
        // Note: If you get "Unsupported format", check if the feature was enabled during vcpkg build.
        LOG_ERROR("[GLUtils] IMG_Load error (", pathStr, "): ", SDL_GetError());
        return img;
    }

    // 3. Use unique_ptr for safe resource management (using SDL_DestroySurface)
    std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> loaded_img(raw_surf, SDL_DestroySurface);

    // 4. Convert surface to the desired format (RGBA32)
    // Note: SDL_PIXELFORMAT_RGBA32 is an alias that ensures correct byte order.
    SDL_Surface* formatted_raw = SDL_ConvertSurface(loaded_img.get(), SDL_PIXELFORMAT_RGBA32);

    if (!formatted_raw) {
        LOG_ERROR("[GLUtils] Conversion error: ", SDL_GetError());
        return img;
    }

    std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> formattedSurf(formatted_raw, SDL_DestroySurface);

    // 5. Transfer pixel data to the ImageRGBA object
    img.Assign(reinterpret_cast<const std::uint32_t*>(formattedSurf->pixels),
        formattedSurf->w,
        formattedSurf->h);

    if (needsFlip) {
        invert_image_RGBA(img);
    }

    return img;
}

void CleanOGLObject( OGLObject& ObjectGPU )
{
	glDeleteBuffers(1,      &ObjectGPU.vboID);
	ObjectGPU.vboID = 0;
	glDeleteBuffers(1,      &ObjectGPU.iboID);
	ObjectGPU.iboID = 0;
	glDeleteVertexArrays(1, &ObjectGPU.vaoID);
	ObjectGPU.vaoID = 0;
}
