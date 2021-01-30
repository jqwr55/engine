#pragma once

#include <glew.h>
#include <glfw3.h>

#include <iostream>

#define DEBUG_BUILD 1

#if DEBUG_BUILD == 1
    #define LOGASSERT(x , y ) if( !(x) ) {std::cout << #x << " " << y << " " << __FILE__ << " " << __LINE__ << std::endl; __builtin_trap(); }
    #define ASSERT(x) if( !(x) ) __builtin_trap();
    #define GLCall(x) GLClearError(); x; ASSERT(GLLogCall(#x , __FILE__ , __LINE__))
    #define GLCallNoCrash(x,error) GLClearError(); x; error = (GLLogCall(#x , __FILE__ , __LINE__) == false ? false : error )
    #define GLFWCall(x) GLFWclearError(); x; ASSERT(GLFWlogCall(#x , __FILE__ , __LINE__));
#else
    #define GLCall(x) x
    #define ASSERT(x) x;
    #define GLFWCall(x) x
    #define LOGASSERT(x , y)
    #define GLCallNoCrash(x,error) x
#endif

void GLFWclearError();
bool GLFWlogCall(const char* function, const char* file, int line);

void GLClearError();
bool GLLogCall(const char* function, const char* file, int line);