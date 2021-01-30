#pragma once
#include <types.h>

struct Renderer;
struct RendererSettingsState;
struct RenderState;

struct Light {
    glm::vec4 pos;
};
struct Texture {
    u32 glID;
};
struct Mesh {
    u32 id;
};
struct Vertex {
    f32 x,y,z;
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
    typedef void (*key_call_back_t)(void* u);
    key_call_back_t func;
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

extern "C" const u32 SIZEOF_RENDERER();
extern "C" const u32 SIZEOF_RENDER_SETINGS_STATE();
extern "C" const u32 SIZEOF_RENDER_STATE();

extern "C" Renderer* ConstructRenderer(void* mem , RendererArgs* args);
extern "C" void DestroyRenderer(Renderer* renderer);

extern "C" void SetRendererSettingsState(Renderer* renderer , RendererSettingsState* state);
extern "C" RendererSettingsState* GetRendererSettingsState(Renderer* renderer , void* mem);
extern "C" void DumpRendererSettingsCout(RendererSettingsState* settings);
extern "C" void SetViewPort(Renderer* renderer , u32 x , u32 y , u32 width , u32 height);

extern "C" void PollEvents(Renderer* r);

extern "C" void SetCursorPos(Renderer* renderer, double x, double y);
extern "C" void GetCursorPos(Renderer* renderer , double* x, double* y);

extern "C" bool ReadyToRender(Renderer* renderer);

extern "C" RenderState* ConstructRenderState(void* mem, Renderer* renderer);
extern "C" void DestroyRenderState(Renderer* renderer , RenderState* state );
extern "C" void BeginRenderState(Renderer* renderer, RenderState* state);
extern "C" void EndRenderState(Renderer* renderer , RenderState* state);
extern "C" CommonShaderParameteres* GetShaderParams(RenderState* state);
extern "C" void ReleaseShaderParams(RenderState* state);
extern "C" void ProcessState(RenderState* state);
extern "C" void SubmitState(Renderer* renderer , RenderState* state);

extern "C" void StartRenderer(Renderer* renderer);
extern "C" void StopRenderer(Renderer* renderer);

extern "C" void InitTexture(Texture* texture);
extern "C" void FreeTexture(Texture* texture);
extern "C" void TextureStorage(Texture* texture , u32 type , u32 internalFormat , u32 mipLevels, u32 x,u32 y,u32 z);
extern "C" void TextureUpload(Texture* texture ,u32 type, void* data , u32 dataFormat ,u32 dataType ,u32 mipLevel,u32 x,u32 y,u32 z);
extern "C" void GenerateMipMapChain(Texture* texture);
extern "C" void SetTextureParamaters(Texture* texture , u32 pname[] , i32 params[] , u32 size);

extern "C" u32 MakeTextureResident (Renderer* renderer , Texture* texture);
extern "C" void MakeTextureNonResident(Renderer* renderer , u32 index);

extern "C" Mesh RegisterMesh(Renderer* renderer, void* verts , u32 vertSize , u32* indecies , u32 indSize , bool shadowFlag );
extern "C" void UnRegisterMesh(Renderer* renderer, Mesh mesh);

extern "C" void DrawMeshes(RenderState* state, Mesh* handles,u32* textureIndex , Transform* transforms ,u32 count);
extern "C" void DrawLines(RenderState* state, Line* lines , u32 count);