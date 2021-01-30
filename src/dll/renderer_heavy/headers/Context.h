#pragma once
#include <iostream>
#include <mutex>
#include <Debug.h>

class Context;

struct InitContext {
    Context* other;
    std::string title;
    unsigned int HEIGHT;
    unsigned int WIDTH;
    unsigned int glVersionMajor;
    unsigned int glVersionMinor;
    bool vSync;
    bool visible;
    bool debug;
    bool suppressed;
};

class Context {
    public:
        Context( const InitContext&& config );
        Context( const InitContext& config );
        ~Context();

        Context( const Context&& other ) = delete;
        Context( const Context& other ) = delete;

        bool ShouldClose();
        void Bind();
        void UnBind();
        void Close();
        void Open();
        void Resize(uint32_t w , uint32_t h);
        void SetDims(int32_t w , int32_t h);
        void SetCallBackPtr(void* ptr);
        void SwapBuffers();
        
        void UpdateDims();
        void SetWindowCloseCallBack(GLFWwindowclosefun callBack);
        void SetWindowSizeCallBack(GLFWwindowsizefun callBack);
        void SetMousePosCallBack(GLFWcursorposfun callBack);
        void SetMouseButtonCallBack(GLFWmousebuttonfun callBack);
        void SetMouseScrollCallBack(GLFWscrollfun callBack);
        void SetKeyCallBack(GLFWkeyfun callBack);
        void SetCharCallBack(GLFWcharfun callBack);


        GLFWwindow* GetContext();
        static void InitGLFW();
        static void InitGLEW();

        struct ContextConfig {
            const char* title;
            uint16_t HEIGHT = 0;
            uint16_t WIDTH = 0;
            uint8_t flags = 0;
        };
        const ContextConfig& GetConfig();
        GLFWwindow* context;
    private:
        ContextConfig config;

        static std::mutex contextLck;
        static int contextCount;
};