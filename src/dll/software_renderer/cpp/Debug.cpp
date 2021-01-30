#include <iostream>
#include <Debug.h>

void GLClearError() {
    while( glGetError()!= GL_NO_ERROR );
}

bool GLLogCall(const char* function, const char* file, int line) {
    while(GLenum error = glGetError()) {

        std::string errorCode;
        switch (error) {
        case GL_INVALID_ENUM:
            errorCode = "GL_INVALID_ENUM";
            break;
        case GL_INVALID_VALUE:
            errorCode = "GL_INVALID_VALUE";
            break;
        case GL_INVALID_OPERATION:
            errorCode = "GL_INVALID_OPERATION";
            break;
        case GL_STACK_OVERFLOW:
            errorCode = "GL_STACK_OVERFLOW";
            break;
        case GL_STACK_UNDERFLOW:
            errorCode = "GL_STACK_UNDERFLOW";
            break;
        case GL_OUT_OF_MEMORY:
            errorCode = "GL_OUT_OF_MEMORY";
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            errorCode = "GL_INVALID_FRAMEBUFFER_OPERATION";
            break;
        case GL_CONTEXT_LOST:
            errorCode = "GL_CONTEXT_LOST";
            break;
        case GL_TABLE_TOO_LARGE:
            errorCode = "GL_TABLE_TOO_LARGE";
            break;
        
        default:
            errorCode = error;
            break;
        }

        std::cout << "[Opengl] runtime error: " << errorCode << " " << function << " " << line << " " << file << std::endl;
        return false;
    }
    return true;
}


void GLFWclearError() {
    const char* description;
    while( glfwGetError(&description) != GLFW_NO_ERROR );
}

bool GLFWlogCall(const char* function, const char* file, int line) {
    const char* description;
    while(auto error = glfwGetError(&description) ) {

        std::string errorCode;
        switch (error) {
            case GLFW_NO_ERROR:
                errorCode = "GLFW_NO_ERROR";
                break;

            case GLFW_NOT_INITIALIZED:
                errorCode = "GLFW_NOT_INITIALIZED";
                break;

            case GLFW_NO_CURRENT_CONTEXT:
                errorCode = "GLFW_NO_CURRENT_CONTEXT";
                break;

            case GLFW_INVALID_ENUM:
                errorCode = "GLFW_INVALID_ENUM";
                break;

            case GLFW_INVALID_VALUE:
                errorCode = "GLFW_INVALID_VALUE";
                break;

            case GLFW_OUT_OF_MEMORY:
                errorCode = "GLFW_OUT_OF_MEMORY";
                break;

            case GLFW_API_UNAVAILABLE:
                errorCode = "GLFW_API_UNAVAILABLE";
                break;

            case GLFW_VERSION_UNAVAILABLE:
                errorCode = "GLFW_VERSION_UNAVAILABLE";
                break;

            case GLFW_PLATFORM_ERROR:
                errorCode = "GLFW_PLATFORM_ERROR";
                break;

            case GLFW_FORMAT_UNAVAILABLE :
                errorCode = "GLFW_FORMAT_UNAVAILABLE";
                break;

            case GLFW_NO_WINDOW_CONTEXT  :
                errorCode = "GLFW_NO_WINDOW_CONTEXT";
                break;


            default:
                errorCode = error;
                break;
        }

        std::cout << "[GLFW] runtime error: " << errorCode << " " << function << " " << line << " " << file << " " << description << std::endl;
        return false;
    }
    return true;
}