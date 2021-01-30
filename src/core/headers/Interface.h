#include <Common.h>

struct Renderer;
struct RendererSettingsState;
struct RenderState;

struct Light {
    glm::vec4 pos;
};
struct Texture {
    u32 handle;
};
struct Mesh {
    u32 id;
};
struct Vertex {
    glm::vec3 p;
    f32 u,v;
};
struct ColoredVertex {
    glm::vec3 pos;
    glm::vec3 col;
};
struct Line {
    ColoredVertex a;
    ColoredVertex b;
};
struct Transform {
    glm::vec3 p;
    glm::vec3 r;
    glm::vec3 s;
};

struct CommonShaderParameteres {
    glm::mat4 projectionViewMatrix;
    glm::mat4 inverseProjectionViewMatrix;
    glm::vec4 viewDir;
    glm::vec4 viewPos;
    glm::vec4 viewRight;
    glm::vec4 viewUp;
};

struct KeyCallBack {
    typedef void(*call_back_t)(void* u);
    call_back_t func;
    void* user;
    i32 key;
};
struct RendererArgs {

    i32 windowWidth;
    i32 windowHeight;
    const char* windowName;

    KeyCallBack* callbacks;
    i32 callbackCount;

    void* client;
    void (*OnCursorPos)(void* client , double x , double y);
    void* (*AllocateAligned)(void* client,u32 alignment,u32 size);
    void* (*Allocate)(void* client,u32 size);
    void (*Free)(void* client,void* mem);
    void (*NotifyClientWindowClose)(void* client);
    void(*OnResize)(void* u ,u32 x,u32 y);
};

struct RendererInterface {
    void* handle = nullptr;

    const u32 (*SIZEOF_RENDERER)();
    const u32 (*SIZEOF_RENDERER_SETINGS_STATE)();
    const u32 (*SIZEOF_RENDER_STATE)();

    Renderer* (*ConstructRenderer)(void* mem , RendererArgs* args);
    void (*DestroyRenderer)(Renderer* renderer);

    void (*SetRendererSettingsState)(Renderer* renderer , RendererSettingsState* state);
    RendererSettingsState* (*GetRendererSettingsState)(Renderer* renderer , void* mem);
    void (*DumpRendererSettingsCout)(RendererSettingsState* settings);
    void (*SetViewPort)(Renderer* renderer , u32 x , u32 y , u32 width , u32 height);
    
    void (*PollEvents)(Renderer* r);
    void (*SetCursorPos)(Renderer* renderer, double x, double y);
    void (*GetCursorPos)(Renderer* renderer , double* x, double* y);
    bool (*ReadyToRender)(Renderer* renderer);
    RenderState* (*ConstructRenderState)(void* mem, Renderer* renderer);
    void (*DestroyRenderState)(Renderer* renderer , RenderState* state );
    void (*BeginRenderState)(Renderer* renderer, RenderState* state);
    void (*EndRenderState)(Renderer* renderer , RenderState* state);
    CommonShaderParameteres* (*GetShaderParams)(RenderState* state);
    void (*ReleaseShaderParams)(RenderState* state);
    
    void (*ProcessState)(RenderState* state);
    void (*SubmitState)(Renderer* renderer , RenderState* state);
    void (*StartRenderer)(Renderer* renderer);
    void (*StopRenderer)(Renderer* renderer);

    void (*InitTexture)(Texture* texture);
    void (*FreeTexture)(Texture* texture);
    void (*TextureStorage)(Texture* texture , u32 type , u32 internalFormat , u32 mipLevels, u32 x,u32 y,u32 z);
    void (*TextureUpload)(Texture* texture ,u32 type, void* data , u32 dataFormat ,u32 dataType ,u32 mipLevel,u32 x,u32 y,u32 z);
    void (*GenerateMipMapChain)(Texture* texture);
    void (*SetTextureParamaters)(Texture* texture , u32 pname[] , i32 params[] , u32 size);
    u32 (*MakeTextureResident) (Renderer* renderer , Texture* texture);
    void (*MakeTextureNonResident)(Renderer* renderer , u32 index);

    Mesh (*RegisterMesh)(Renderer* renderer, void* verts , u32 vertSize , u32* indecies , u32 indSize , bool shadowFlag );
    void (*UnRegisterMesh)(Renderer* renderer, Mesh mesh);

    void (*DrawLines)(RenderState* state, Line* lines , u32 count);
    void (*DrawMeshes)(RenderState* state, Mesh* handles,u32* textureIndex , Transform* transforms ,u32 count);

    std::filesystem::file_time_type time;
};

void LoadRendererInterface(RendererInterface& interface , const char* path) {
    
    interface.time = std::filesystem::last_write_time(path);

    std::cout << "Opening: " << path << std::endl;
    interface.handle = dlopen(path , RTLD_LAZY);
    if(!interface.handle) {
        std::cerr << "Cannot open renderer " << dlerror() << std::endl;
        exit(1);
    }
    std::cout << "Loading symbols" << std::endl;
    LDClearErrors();

    LDCall(interface.SIZEOF_RENDERER = (decltype(interface.SIZEOF_RENDERER))dlsym(interface.handle ,"SIZEOF_RENDERER") );
    LDCall(interface.SIZEOF_RENDERER_SETINGS_STATE = (decltype(interface.SIZEOF_RENDERER_SETINGS_STATE))dlsym(interface.handle ,"SIZEOF_RENDERER_SETINGS_STATE") );
    LDCall(interface.SIZEOF_RENDER_STATE = (decltype(interface.SIZEOF_RENDER_STATE))dlsym(interface.handle ,"SIZEOF_RENDER_STATE") );

    LDCall(interface.ConstructRenderer = (decltype(interface.ConstructRenderer))dlsym(interface.handle ,"ConstructRenderer") );
    LDCall(interface.DestroyRenderer = (decltype(interface.DestroyRenderer))dlsym(interface.handle ,"DestroyRenderer") );
    LDCall(interface.SetViewPort = (decltype(interface.SetViewPort))dlsym(interface.handle ,"SetViewPort") );
    LDCall(interface.SetRendererSettingsState = (decltype(interface.SetRendererSettingsState))dlsym(interface.handle ,"SetRendererSettingsState") );
    LDCall(interface.GetRendererSettingsState = (decltype(interface.GetRendererSettingsState))dlsym(interface.handle ,"GetRendererSettingsState") );
    LDCall(interface.DumpRendererSettingsCout = (decltype(interface.DumpRendererSettingsCout))dlsym(interface.handle ,"DumpRendererSettingsCout") );
    LDCall(interface.PollEvents = (decltype(interface.PollEvents))dlsym(interface.handle ,"PollEvents") );
    LDCall(interface.SetCursorPos = (decltype(interface.SetCursorPos))dlsym(interface.handle ,"SetCursorPos") );
    LDCall(interface.GetCursorPos = (decltype(interface.GetCursorPos))dlsym(interface.handle ,"GetCursorPos") );
    LDCall(interface.ReadyToRender = (decltype(interface.ReadyToRender))dlsym(interface.handle ,"ReadyToRender") );
    LDCall(interface.ConstructRenderState = (decltype(interface.ConstructRenderState))dlsym(interface.handle ,"ConstructRenderState") );
    LDCall(interface.DestroyRenderState = (decltype(interface.DestroyRenderState))dlsym(interface.handle ,"DestroyRenderState") );
    LDCall(interface.BeginRenderState = (decltype(interface.BeginRenderState))dlsym(interface.handle ,"BeginRenderState") );
    LDCall(interface.EndRenderState = (decltype(interface.EndRenderState))dlsym(interface.handle ,"EndRenderState") );
    LDCall(interface.GetShaderParams = (decltype(interface.GetShaderParams))dlsym(interface.handle ,"GetShaderParams") );
    LDCall(interface.ReleaseShaderParams = (decltype(interface.ReleaseShaderParams))dlsym(interface.handle ,"ReleaseShaderParams") );
    LDCall(interface.ProcessState = (decltype(interface.ProcessState))dlsym(interface.handle ,"ProcessState") );
    LDCall(interface.SubmitState = (decltype(interface.SubmitState))dlsym(interface.handle ,"SubmitState") );
    LDCall(interface.StartRenderer = (decltype(interface.StartRenderer))dlsym(interface.handle ,"StartRenderer") );
    LDCall(interface.StopRenderer = (decltype(interface.StopRenderer))dlsym(interface.handle ,"StopRenderer") );
    LDCall(interface.InitTexture = (decltype(interface.InitTexture))dlsym(interface.handle ,"InitTexture") );
    LDCall(interface.FreeTexture = (decltype(interface.FreeTexture))dlsym(interface.handle ,"FreeTexture") );
    LDCall(interface.TextureStorage = (decltype(interface.TextureStorage))dlsym(interface.handle ,"TextureStorage") );
    LDCall(interface.TextureUpload = (decltype(interface.TextureUpload))dlsym(interface.handle ,"TextureUpload") );
    LDCall(interface.TextureUpload = (decltype(interface.TextureUpload))dlsym(interface.handle ,"TextureUpload") );
    LDCall(interface.GenerateMipMapChain = (decltype(interface.GenerateMipMapChain))dlsym(interface.handle ,"GenerateMipMapChain") );
    LDCall(interface.SetTextureParamaters = (decltype(interface.SetTextureParamaters))dlsym(interface.handle ,"SetTextureParamaters") );
    
    LDCall(interface.MakeTextureResident = (decltype(interface.MakeTextureResident))dlsym(interface.handle ,"MakeTextureResident") );
    LDCall(interface.MakeTextureNonResident = (decltype(interface.MakeTextureNonResident))dlsym(interface.handle ,"MakeTextureNonResident") );
    
    LDCall(interface.UnRegisterMesh = (decltype(interface.UnRegisterMesh))dlsym(interface.handle ,"UnRegisterMesh") );
    LDCall(interface.RegisterMesh = (decltype(interface.RegisterMesh))dlsym(interface.handle ,"RegisterMesh") );

    LDCall(interface.DrawLines = (decltype(interface.DrawLines))dlsym(interface.handle ,"DrawLines") );
    LDCall(interface.DrawMeshes = (decltype(interface.DrawMeshes))dlsym(interface.handle ,"DrawMeshes") );
}

void UnloadRendererInterface(RendererInterface& interface) {
    LDCall(dlclose(interface.handle));
    for(u32 i = 0 ; i < sizeof(interface) ; i++) {
        ((byte*)&interface)[i] = 0;
    }
}


struct T0 {
    i32 c;
};

struct T {
    T0 w;
    u32 a;
};

introspect(x) struct T1 {
    u32 b;
    T c;
};