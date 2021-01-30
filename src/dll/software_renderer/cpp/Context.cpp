#include <atomic>
#include <Context.h>

void TerminateGlfw() {
    glfwTerminate();
}

std::mutex Context::contextLck;

Context::Context( const InitContext&& c ) {

    std::lock_guard<std::mutex>lck(contextLck);
    contextCount++;
    if( contextCount == 1 ) {
        InitGLFW();
    }

    config.title = c.title.c_str();
    config.HEIGHT = c.HEIGHT;
    config.WIDTH = c.WIDTH;
    
    config.flags |= c.debug << 0;
    config.flags |= c.visible << 1;
    config.flags |= c.suppressed << 2;
    config.flags |= (c.vSync << 3) & 0b11111000;

    GLFWCall(glfwWindowHint(GLFW_CONTEXT_NO_ERROR, !c.debug ));
    GLFWCall(glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, c.debug ));

    GLFWCall(glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE));
    GLFWCall(glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, c.glVersionMajor));
    GLFWCall(glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, c.glVersionMinor));

    GLFWCall(glfwWindowHint(GLFW_VISIBLE, c.visible));

    GLFWCall(context = glfwCreateWindow(config.WIDTH , config.HEIGHT , config.title , NULL  , c.other != nullptr ? c.other->context : NULL ));

    if( !context ) {
        const char* description;
        int error = glfwGetError( &description );
        std::string Err(  "Failed to create GLFW context " );
        throw std::runtime_error( Err + description );
    }

    GLFWCall(glfwMakeContextCurrent(0));
    GLFWCall(glfwMakeContextCurrent(context));
    GLFWCall(glfwSwapInterval( !c.vSync ));

    if( contextCount == 1 ) {
        InitGLEW();
    }

    if( !c.suppressed ) {
        std::cout << "SHADING_VERSION ";
        GLFWCall(std::cout << glGetString(GL_SHADING_LANGUAGE_VERSION) << "  ");
        GLFWCall(std::cout << glGetString(GL_RENDERER) << "  ");
        GLFWCall(std::cout << glGetString(GL_VERSION) << " Context initialized " << config.title << " " << context  << std::endl);
    }
}

Context::Context( const InitContext& c ) {

    std::lock_guard<std::mutex>lck(contextLck);
    contextCount++;
    if( contextCount == 1 ) {
        InitGLFW();
    }
    
    config.title = c.title.c_str();
    config.HEIGHT = c.HEIGHT;
    config.WIDTH = c.WIDTH;
    
    config.flags |= c.debug << 0;
    config.flags |= c.visible << 1;
    config.flags |= c.suppressed << 2;
    config.flags |= (c.vSync << 3) & 0b11111000;

    GLFWCall(glfwWindowHint(GLFW_CONTEXT_NO_ERROR, !c.debug ));
    GLFWCall(glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, c.debug ));

    GLFWCall(glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE));
    GLFWCall(glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, c.glVersionMajor));
    GLFWCall(glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, c.glVersionMinor));

    GLFWCall(context = glfwCreateWindow(config.WIDTH , config.HEIGHT , config.title , NULL  , c.other != nullptr ? c.other->context : NULL ));

    if( !context ) {
        const char* description;
        int error = glfwGetError( &description );
        std::string Err( "Failed to create GLFW context " );
        throw std::runtime_error( Err + description );
    }

    GLFWCall(glfwMakeContextCurrent(0));
    GLFWCall(glfwMakeContextCurrent(context));
    GLFWCall(glfwSwapInterval( 1 ));

    if( contextCount == 1 ) {
        InitGLEW();
    }

    if( !c.suppressed ) {
        std::cout << "SHADING_VERSION ";
        GLFWCall(std::cout << glGetString(GL_SHADING_LANGUAGE_VERSION) << "  ");
        GLFWCall(std::cout << glGetString(GL_RENDERER) << "  ");
        GLFWCall(std::cout << glGetString(GL_VERSION) << " Context initialized " << config.title << " " << context  << std::endl);
    }
}

Context::~Context() {
    std::lock_guard<std::mutex>lck(contextLck);
    GLFWCall(glfwDestroyWindow(context));
    contextCount--;
    if( contextCount == 0 ) {
        glfwTerminate();
    }
}

void Context::SwapBuffers() {
    GLFWCall(glfwSwapBuffers(context));
}
void Context::Bind() {
    GLFWCall(glfwMakeContextCurrent(context));
}
void Context::UnBind() {
    GLFWCall(glfwMakeContextCurrent(NULL));
}

bool Context::ShouldClose() {
    GLFWCall(return glfwWindowShouldClose(context));
}

void Context::Close() {
    GLFWCall(glfwSetWindowShouldClose( context , GLFW_TRUE ));
}
void Context::Open() {
    GLFWCall(glfwSetWindowShouldClose( context , GLFW_FALSE ));
}
void Context::Resize(uint32_t w , uint32_t h) {
    GLFWCall(glfwSetWindowSize(context , w , h));
}
void Context::SetCallBackPtr(void* ptr) {
    GLFWCall(glfwSetWindowUserPointer(context , ptr ));
}

void Context::SetWindowCloseCallBack(GLFWwindowclosefun callBack) {
    GLFWCall(glfwSetWindowCloseCallback(context , callBack));
}
void Context::SetWindowSizeCallBack(GLFWwindowsizefun callBack) {
    GLFWCall(glfwSetWindowSizeCallback(context , callBack));
}
void Context::SetMousePosCallBack(GLFWcursorposfun callBack) {
    GLFWCall(glfwSetCursorPosCallback(context , callBack));
}
void Context::SetMouseScrollCallBack(GLFWscrollfun callBack) {
    GLFWCall(glfwSetScrollCallback(context , callBack));
}
void Context::SetKeyCallBack(GLFWkeyfun callBack) {
    GLFWCall(glfwSetKeyCallback(context , callBack));
}

void Context::SetMouseButtonCallBack(GLFWmousebuttonfun callBack) {
    GLFWCall(glfwSetMouseButtonCallback(context , callBack));
}
void Context::SetCharCallBack(GLFWcharfun callBack) {
    GLFWCall(glfwSetCharCallback(context, callBack));
}
void Context::SetDims(int32_t w , int32_t h) {
    config.WIDTH = w;
    config.HEIGHT = h;
}
void Context::UpdateDims() {
    int h,w;
    GLFWCall(glfwGetWindowSize(context , &w , &h));
    config.WIDTH = w;
    config.HEIGHT = h;
}

const Context::ContextConfig& Context::GetConfig() {
    return config;
}

GLFWwindow* Context::GetContext() {
    return context;
}

void Context::InitGLFW() {
    int init = glfwInit();

    std::cout << "GLFW initialized " << glfwGetVersionString() << std::endl;

    if( init == GLFW_FALSE) {
        const char* description;
        int error = glfwGetError( &description );
        
        std::string errMsg( "Failed to initialize GLFW , with error code: " + error );
        errMsg += " error description: ";

        throw std::runtime_error( errMsg + description );
    }
}

void Context::InitGLEW() {
    unsigned int init = glewInit();

    if(init != GLEW_OK) {
        std::cout << "GLEW Error: " << glewGetErrorString(init) << std::endl;
        throw std::runtime_error( "Failed to initialize GLEW" );
    }
}

int Context::contextCount = 0;