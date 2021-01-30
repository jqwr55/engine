#pragma once

#include <string>
#include <vector>

#include <Debug.h>


struct ShaderSource {
    std::string vertexShader = "null";
    std::string geometryShader = "null";
    std::string fragmentShader = "null";
    std::string computeShader = "null";
};

struct SubroutineSelection {
    const char* name;
    short index;
};

class Shader {
    public:
        Shader() {}
        ~Shader();

        Shader(Shader&& other) = delete;
        Shader(const Shader& other) = delete;
        Shader& operator = (const Shader& other) = delete;
        Shader& operator = (Shader&& other) = delete;

        void MakeShader(std::string filepath , const char* varnames[] = nullptr , const unsigned int size = 0);
        void WriteBinaryIntoFile(std::string filePath);
        void Bind();
        void UnBind();
        const unsigned int& GetShaderID();

    private:
        ShaderSource LoadShader(const std::string filepath);
        unsigned int CreateShader(const ShaderSource& source , const std::string& filename , const char* feedbacknames[] , int size = 0);
        unsigned int CompileShader(unsigned int type, const std::string& source , const std::string& filename);

        unsigned int shaderid;
};