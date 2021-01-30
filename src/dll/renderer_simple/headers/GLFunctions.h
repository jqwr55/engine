#pragma once

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <types.h>
#include <Debug.h>

#include <TextureObject.h>


std::string Mygetline2(const std::string& str, int line) {
    size_t pos = 0;
    
    if(line < 0) {
        return std::string();
    }

    while( ( line-- > 0 ) && ( pos < str.length() ) ) {
        pos = str.find("\n", pos) + 1;
    }
    
    if (pos >= str.length()) {
        return std::string();
    }
    
    size_t end = str.find("\n", pos);
    return str.substr(pos, (end == std::string::npos ? std::string::npos : (end - pos + 1)));
}


u32 GLCompileShader(u32 type , const std::string& source) {
    
    GLCall(u32 gl_handle = glCreateShader(type));
    const char* src = source.c_str();
    GLCall(glShaderSource(gl_handle, 1, &src, nullptr));
    GLCall(glCompileShader(gl_handle));

    i32 result;
    GLCall(glGetShaderiv(gl_handle, GL_COMPILE_STATUS, &result));

    if(result == GL_FALSE) {
        i32 lenght;
        GLCall(glGetShaderiv(gl_handle , GL_INFO_LOG_LENGTH, &lenght));
        char message[lenght];
        GLCall(glGetShaderInfoLog(gl_handle, lenght, &lenght, message));
        std::cout << std::endl;

        switch( type ) {
            case GL_VERTEX_SHADER:
                std::cout <<"\x1B[36m Failed to compile the \x1B[0m \x1B[35m vertex \x1B[0m \x1B[36m shader \x1B[0m" <<std::endl;
                break;

            case GL_FRAGMENT_SHADER:
                std::cout <<"\x1B[36m Failed to compile the \x1B[0m \x1B[35m fragment \x1B[0m \x1B[36m shader \x1B[0m" <<std::endl;
                break;

            case GL_GEOMETRY_SHADER:
                std::cout <<"\x1B[36m Failed to compile the \x1B[0m \x1B[35m geometry \x1B[0m \x1B[36m shader \x1B[0m" <<std::endl;
                break;

            default:
                std::cout <<"\x1B[36m Failed to compile shader of type:\x1B[0m \x1B[35m" << type << "\x1B[0m" <<std::endl;
                break;
        }


        {
            std::string a = message;
            int l = 0;
            std::vector<int>j;
            while(Mygetline2(a , l).size() != 0) {
                if(Mygetline2(a , l)[3] != ')') {
                    if(Mygetline2(a , l)[4] != ')') {
                        j.push_back( ((int)(Mygetline2(a , l)[2]) - 48) * 100 + ((int)(Mygetline2(a , l)[3]) - 48) * 10 + ((int)(Mygetline2(a , l)[4]) - 48));
                    }
                    else {
                        j.push_back( ((int)(Mygetline2(a , l)[2]) - 48) * 10 + ((int)(Mygetline2(a , l)[3]) - 48));
                    }
                }
                else {
                    j.push_back((int)(Mygetline2(a , l)[2]) - 48);
                }

                l++;
            }

            
            std::vector<int>::iterator ptr = j.begin(); 
            std::cout << std::endl;
            
            l = 0;
            while(Mygetline2(source , l ).size() != 0) {
                if((*ptr) == l + 1) {
                    ptr++;
                    std::cout << "\x1B[31m" << " " << l+1 <<". " << Mygetline2(source ,l).substr(0 , Mygetline2(source ,l).size() - 1) << "\x1B[4;#m" << std::endl;
                } 
                else {
                    std::cout <<"\x1B[32m "<<l+1 <<". " << Mygetline2(source ,l).substr(0 , Mygetline2(source ,l).size() - 1) << "\x1B[0m" << std::endl;
                }
                l++;
            }

            std::cout << std::endl << "\x1B[33m" << message << "\x1B[0m" <<std::endl;
        }

        GLCall(glDeleteShader(gl_handle));
        return 0;
    }
    return gl_handle;
}

void MemCpy(void* _RESTRICT src , void* _RESTRICT dst , u32 size) {
    for(u32 i = 0; i < size; i++) {
        ((byte*)dst)[i] = ((byte*)src)[i];
    }
}

u32 FindStr(char* source , const char* find) {

    u32 fIndex = 0;
    u32 index = 0;
    u32 ret = 0xFFFFFFFF;
    bool begin = false;


    while( source[index] != 0 ) {
        if( source[index] == find[0] ) {
            ret = index;
            begin = true;
        }

        if(begin) {
            while( source[index] == find[fIndex] ) {
                index++;
                fIndex++;
            }

            if( find[fIndex] == 0 ) {
                return ret;
            }
            else {
                fIndex = 0;
                begin = false;
                ret = 0xFFFFFFFF;
            }
        }
        else {
            index++;
        }

    }

    return 0xFFFFFFFF;
}

struct Shaders {
    std::string vertex;
    std::string fragment;
    std::string greometry;
    std::string compute;
};
void LoadShader(const std::string& filePath , Shaders& out) {

    std::ifstream stream(filePath);
    if( stream.fail() ) {
        std::cerr << filePath << " error opening" << std::endl;
    }

    out.vertex.clear();
    out.fragment.clear();
    out.greometry.clear();
    out.compute.clear();

    std::string line;
    std::string stageName;
    u32 mode = GL_NONE;
    while( getline(stream, line) ) {

        if( line.find("#shader:") != std::string::npos ) {
            stageName = line.substr( line.find("#shader:") );
            if( line.find("vertex") != std::string::npos ) {
                mode = GL_VERTEX_SHADER;
                continue;
            }
            else if( line.find("fragment") != std::string::npos ) {
                mode = GL_FRAGMENT_SHADER;
                continue;
            }
            else if( line.find("geometry") != std::string::npos ) {
                mode = GL_GEOMETRY_SHADER;
                continue;
            }
            else if( line.find("compute") != std::string::npos) {
                mode = GL_COMPUTE_SHADER;
                continue;
            }
            else {
                mode = GL_NONE;
                continue;
            }
        }

        switch ( mode ) {
            case GL_VERTEX_SHADER :
                out.vertex = out.vertex + line + "\n";
                break;

            case GL_FRAGMENT_SHADER :
                out.fragment = out.fragment + line + "\n";
                break;

            case GL_GEOMETRY_SHADER :
                out.greometry = out.greometry + line + "\n";
                break;
            case GL_COMPUTE_SHADER :
                out.compute = out.greometry + line + "\n";
                break;
            default:
                std::cerr << "No shader stages found in file or unrecognised shader stage type in file: " << filePath << " with type: " << mode << std::endl;
                stream.close();
                return;
                break;
        }
    }

    stream.close();
}

u32 CreateShaderHandle(const std::string& filePath , const char* fnames[] , int size) {


    GLCall(u32 program_handle = glCreateProgram());

    Shaders source;
    LoadShader(filePath , source);

    u32 gl_vertex_handle = 0;
    u32 gl_fragment_handle = 0;
    u32 gl_geometry_handle = 0;
    u32 gl_compute_handle = 0;
    bool anyError = false;

    if(source.vertex.size() != 0) {
        gl_vertex_handle = GLCompileShader(GL_VERTEX_SHADER , source.vertex);
        if( gl_vertex_handle != 0 ) {
            GLCall(glAttachShader(program_handle , gl_vertex_handle));
        }
        else {
            anyError = true;
        }
    }
    if(source.fragment.size() != 0) {
        gl_fragment_handle = GLCompileShader(GL_FRAGMENT_SHADER , source.fragment);
        if( gl_fragment_handle != 0 ) {
            GLCall(glAttachShader(program_handle , gl_fragment_handle));
        }
        else {
            anyError = true;
        }
    }
    if(source.greometry.size() != 0) {
        gl_geometry_handle = GLCompileShader(GL_GEOMETRY_SHADER , source.greometry);
        if( gl_geometry_handle != 0 ) {
            GLCall(glAttachShader(program_handle , gl_geometry_handle));
        }
        else {
            anyError = true;
        }
    }
    if(source.compute.size() != 0) {
        gl_compute_handle = GLCompileShader(GL_COMPUTE_SHADER , source.compute);
        if( gl_compute_handle != 0 ) {
            GLCall(glAttachShader(program_handle , gl_compute_handle));
        }
        else {
            anyError = true;
        }
    }

    if(size > 0 ) {
        GLCallNoCrash(glTransformFeedbackVaryings(program_handle , size , fnames , GL_INTERLEAVED_ATTRIBS) , anyError);
    }

    GLCall(glLinkProgram(program_handle));
    {
        int success;
        GLCall(glGetProgramiv(program_handle , GL_LINK_STATUS , &success));
        if( success != GL_TRUE ) {
            int length;
            GLCall(glGetProgramiv(program_handle , GL_INFO_LOG_LENGTH , &length));
            char message[length];
            GLCall(glGetProgramInfoLog(program_handle ,length , &length,  message ));
            std::cout << "Link error in " + filePath + " :"  << message << std::endl;

            anyError = true;
        }
    }

    GLCall(glValidateProgram(program_handle));
    {
        int success;
        GLCall(glGetProgramiv(program_handle , GL_VALIDATE_STATUS , &success));
        if( success != GL_TRUE ) {
            int length;
            GLCall(glGetProgramiv(program_handle , GL_INFO_LOG_LENGTH , &length));
            char message[length];
            GLCall(glGetProgramInfoLog(program_handle ,length , &length,  message ));
            std::cout << "Validation error: " << message << std::endl;

            anyError = true;
        }
    }

    if( gl_compute_handle != 0 ) {
        GLCall(glDetachShader(program_handle , gl_compute_handle));
        GLCall(glDeleteShader(gl_compute_handle));
    }
    if( gl_vertex_handle != 0 ) {
        GLCall(glDetachShader(program_handle , gl_vertex_handle));
        GLCall(glDeleteShader(gl_vertex_handle));
    }
    if( gl_fragment_handle != 0 ) {
        GLCall(glDetachShader(program_handle , gl_fragment_handle));
        GLCall(glDeleteShader(gl_fragment_handle));
    }
    if( gl_geometry_handle != 0 ) {
        GLCall(glDetachShader(program_handle , gl_geometry_handle));
        GLCall(glDeleteShader(gl_compute_handle));
    }

    if(anyError) {
        GLCall(glDeleteProgram(program_handle));
        program_handle = 0;
    }

    return program_handle;
}

BindlessTexture::~BindlessTexture() {
    GLCall(glDeleteTextures(1, &ID));
    ID = 0;
}

uint64_t BindlessTexture::GetAddress() {
    GLCall(return glGetTextureHandleARB(ID));;
}

void BindlessTexture::Init() {
    GLCall(glGenTextures(1, &ID));
}

void BindlessTexture::SetTextureParameters(GLenum pname[] , const GLint params[] , uint32_t size) {
    for(int i = 0; i < size ; i++) {
        GLCall(glTextureParameteri(ID , pname[i] , params[i]));
    }
}

struct Shader {
    u32 program_handle;
    std::filesystem::file_time_type time;
};

struct DrawElementsIndirectCommand {
    u32 count;
    u32 instanceCount;
    u32 firstIndex;
    u32 baseVertex;
    u32 baseInstance;
};