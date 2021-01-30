#include <iostream>
#include <fstream>
#include <Shader.h>

Shader::~Shader() {
    GLCall(glDeleteProgram(shaderid));
}

void Shader::MakeShader(std::string filepath , const char* vars[] , const unsigned int size) {
    ShaderSource shadersource;
    shadersource = LoadShader(filepath);
    shaderid = CreateShader(shadersource , filepath , vars ,size);
}

void Shader::Bind() {
    GLCall(glUseProgram(shaderid));
}

void Shader::UnBind(){
    GLCall(glUseProgram(0));
}

ShaderSource Shader::LoadShader(const std::string filepath) {
    std::string line;
    std::ifstream file;
    ShaderSource shader;
    file.open(filepath);

    if( file.fail() == 1 ) {
        throw std::runtime_error( filepath + " Doesn't exist" );
    }

    std::string stageName;
    unsigned int mode = GL_NONE;
    while( getline(file, line) ) {
        if( line.find("#shader:") != std::string::npos ) {
            stageName = line.substr( line.find("#shader:") );
            if( line.find("vertex") != std::string::npos ) {
                shader.vertexShader.clear();
                mode = GL_VERTEX_SHADER;
                continue;
            }
            else if( line.find("fragment") != std::string::npos ) {
                shader.fragmentShader.clear();
                mode = GL_FRAGMENT_SHADER;
                continue;
            }
            else if( line.find("geometry") != std::string::npos ) {
                shader.geometryShader.clear();
                mode = GL_GEOMETRY_SHADER;
                continue;
            }
            else if( line.find("compute") != std::string::npos) {
                shader.computeShader.clear();
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
                shader.vertexShader = shader.vertexShader + line + "\n";
                break;

            case GL_FRAGMENT_SHADER :
                shader.fragmentShader = shader.fragmentShader + line + "\n";
                break;

            case GL_GEOMETRY_SHADER :
                shader.geometryShader = shader.geometryShader + line + "\n";
                break;
            case GL_COMPUTE_SHADER :
                shader.computeShader = shader.computeShader + line + "\n";
                break;
            default:
                throw std::runtime_error( "No shader stages found in file or unrecognised shader stage type in file: " + filepath + " of type: " + stageName );
                break;
        }
    }

    file.close();
    return shader;
}

unsigned int Shader::CreateShader(const ShaderSource& source , const std::string& filename , const char* fnames[] , int size) {

    GLCall(unsigned int program = glCreateProgram());
    unsigned int vs;
    unsigned int gs;
    unsigned int fs;
    unsigned int cs;
    
    bool compileError = true;

    if(source.vertexShader != "null") {
        vs = CompileShader(GL_VERTEX_SHADER, source.vertexShader , filename);
        compileError = ( vs == 0 ? true : false );
        if( !compileError ) {
            GLCall(glAttachShader(program, vs));
        }
    }


    if(source.fragmentShader != "null") {
        fs = CompileShader(GL_FRAGMENT_SHADER, source.fragmentShader, filename);
        compileError = ( fs == 0 ? true : false );

        if( !compileError ) {
            GLCall(glAttachShader(program, fs));
        }
    }

    if(source.geometryShader != "null") {
        gs = CompileShader(GL_GEOMETRY_SHADER , source.geometryShader, filename);
        compileError = ( gs == 0 ? true : false );
        
        if( !compileError ) {
            GLCall(glAttachShader(program, gs));
        }
    }

    if(source.computeShader != "null") {
        cs = CompileShader(GL_COMPUTE_SHADER , source.computeShader , filename);

        compileError = ( cs == 0 ? true : false );
        
        if( !compileError ) {
            GLCall(glAttachShader(program, cs));
        }
    }

    if( compileError ) {
        throw std::runtime_error( "Shader source was NULL or a compile error occured" );
    }

    if(size > 0 ) {
        GLCall(glTransformFeedbackVaryings(program , size , fnames , GL_INTERLEAVED_ATTRIBS));
    }

    GLCall(glLinkProgram(program));
    {
        int success;
        GLCall(glGetProgramiv(program , GL_LINK_STATUS , &success));
        if( success != GL_TRUE ) {
            int length;
            GLCall(glGetProgramiv(program , GL_INFO_LOG_LENGTH , &length));
            char message[length];
            GLCall(glGetProgramInfoLog(program ,length , &length,  message ));
            std::cout << "Link error in " + filename + " :"  << message << std::endl;
        }
    }

    GLCall(glValidateProgram(program));
    {
        int success;
        GLCall(glGetProgramiv(program , GL_VALIDATE_STATUS , &success));
        if( success != GL_TRUE ) {
            int length;
            GLCall(glGetProgramiv(program , GL_INFO_LOG_LENGTH , &length));
            char message[length];
            GLCall(glGetProgramInfoLog(program ,length , &length,  message ));
            std::cout << "Validation error: " << message << std::endl;
        }
    }

    
    if(source.geometryShader != "null") {
        GLCall(glDetachShader(program, gs ));
        GLCall(glDeleteShader(gs));
    }
    if(source.fragmentShader != "null") {
        GLCall(glDetachShader(program, fs ));
        GLCall(glDeleteShader(fs));
    }
    if(source.vertexShader != "null") {
        GLCall(glDetachShader(program, vs ));
        GLCall(glDeleteShader(vs));
    }
    if(source.computeShader != "null") {
        GLCall(glDetachShader(program, cs ));
        GLCall(glDeleteShader(cs));
    }


    return program;
}

std::string Mygetline(const std::string& str, int line) {
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

unsigned int Shader::CompileShader(unsigned int type, const std::string& source , const std::string& filename) {
    
    GLCall(unsigned int id = glCreateShader(type));
    const char* src = source.c_str();
    GLCall(glShaderSource(id, 1, &src, nullptr));
    GLCall(glCompileShader(id));

    int result;
    GLCall(glGetShaderiv(id, GL_COMPILE_STATUS, &result));

    if(result == GL_FALSE) {
        int lenght;
        GLCall(glGetShaderiv(id , GL_INFO_LOG_LENGTH, &lenght));
        char message[lenght];
        GLCall(glGetShaderInfoLog(id, lenght, &lenght, message));
        std::cout<<std::endl;
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

        std::cout << "In file: " << filename << std::endl;
        
        {
            std::string a = message;
            int l = 0;
            std::vector<int>j;
            while(Mygetline(a , l).size() != 0) {
                if(Mygetline(a , l)[3] != ')') {
                    if(Mygetline(a , l)[4] != ')') {
                        j.push_back( ((int)(Mygetline(a , l)[2]) - 48) * 100 + ((int)(Mygetline(a , l)[3]) - 48) * 10 + ((int)(Mygetline(a , l)[4]) - 48));
                    }
                    else {
                        j.push_back( ((int)(Mygetline(a , l)[2]) - 48) * 10 + ((int)(Mygetline(a , l)[3]) - 48));
                    }
                }
                else {
                    j.push_back((int)(Mygetline(a , l)[2]) - 48);
                }

                l++;
            }

            
            std::vector<int>::iterator ptr = j.begin(); 
            std::cout << std::endl;
            
            l = 0;
            while(Mygetline(source , l ).size() != 0) {
                if((*ptr) == l + 1) {
                    ptr++;
                    std::cout << "\x1B[31m" << " " << l+1 <<". " << Mygetline(source ,l).substr(0 , Mygetline(source ,l).size() - 1) << "\x1B[4;#m" << std::endl;
                } 
                else {
                    std::cout <<"\x1B[32m "<<l+1 <<". " << Mygetline(source ,l).substr(0 , Mygetline(source ,l).size() - 1) << "\x1B[0m" << std::endl;
                }
                l++;
            }

            std::cout << std::endl << "\x1B[33m" << message << "\x1B[0m" <<std::endl;
        }

        GLCall(glDeleteShader(id));
        return 0;
    }

    return id;
}


const unsigned int& Shader::GetShaderID() {
    return shaderid;
}

void Shader::WriteBinaryIntoFile(std::string filePath) {

    int formats = 0;
    GLCall(glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS , &formats));
    if( formats < 1 ) {
        std::cout << "Driver does not support any binary formats" << std::endl;
    }

    int lenght = 0;
    GLCall(glGetProgramiv(shaderid , GL_PROGRAM_BINARY_LENGTH , &lenght));
    uint8_t buffer[lenght];
    GLenum format;
    GLCall(glGetProgramBinary(shaderid , lenght , NULL , &format , buffer));
    std::cout << "Writing to " << filePath << " , binary format: " << format << std::endl;
    std::ofstream out(filePath , std::ios::binary);
    out.write( (char*)buffer , lenght );
    out.close();
}