#pragma once

#include <RendererInterface.h>
#include <GLFunctions.h>
#include <VertexArray.h>
#include <BufferObject.h>
#include <Funcs.h>

#include <unordered_map>
#include <thread>
#include <mutex>

#define introspect(params)

#define VAO_INIT 1
#define VAO_ATTACH 1 << 1

enum VaoNames {
    VAO_DEBUG,
    VAO_GEOMETRY,
    VAO_TEXTURED,

    VAO_COUNT,
};
enum DescriptorNames {
    VERTEX_COL_DESCRIPTOR,
    VERTEX_UV_DESCRIPTOR,
    MATRIX_DESCRIPTOR,
    TEXTURE_MATRIX_DESCIRPTOR,

    DESCRIPTOR_COUNT,
};
enum ShaderNames {
    SHADER_DEBUG,
    SHADER_SHADOW_CAP,
    SHADER_SHADOW_EDGE,
    SHADER_GEOMETRY,

    SHADER_COUNT,
};
enum TextureNames {
    TEXTURE_NULL,

    TEXTURE_COUNT,
};

enum TokenHead {
    TOKEN_VAO_INIT, 
    TOKEN_VAO_DELETE,                               // u32 id
    TOKEN_VAO_ATTACH_TEXTURE_MATRIX_DESCIRPTOR,

    TOKEN_TEXTURE_MAKE_RESIDENT,                    // u64 8 byte address | u32 4 byte index
    TOKEN_TEXTURE_MAKE_NON_RESIDENT,                // u64 8 byte address | u32 4 byte index
    TOKEN_VIEWPORT_SIZE,                            // u32 | u32 | u32 | u32 | 

    TOKEN_COUNT,
};

struct ShaderInfo {
    const char* filePath;
    const char** names;
    i32 nameCount;
};
constexpr ShaderInfo shaderInfoTable[SHADER_COUNT] = {
    {"../res/shaders/DebugShader.glsl",nullptr,0},
    {"../res/shaders/ShadowCapShader.glsl",nullptr,0},
    {"../res/shaders/ShadowEdgeShader.glsl",nullptr,0},
    {"../res/shaders/GeometryShader.glsl",nullptr,0},
};

struct MeshInternal {
    u32 vertexCount;
    u32 vertexOffset;
    u32 indexCount;
    u32 indexOffset;
    
    u32 shadowVertexCount;
    u32 shadwoVertexOffset;
    u32 shadowIndexCount;
    u32 shadowIndexOffset;
};

struct TextureHandleInternal {
    u64 address;
    u32 slotIndex;
};
struct MatrixTextureInstanceData {
    glm::mat4x3 matrix;
    u32 textureIndex;
};

const GLbitfield storage = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_CLIENT_STORAGE_BIT,map = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT;

struct CommonBuffers {
    CommonBuffers() : pinned(32 * MEGA_BYTES ,storage , map) {
        meshVertexBuffer.ImmutableStorage(nullptr , sizeof(Vertex) , 0 );
        //meshShadowVertexBuffer.ImmutableStorage(nullptr , sizeof(Vertex) , 0);
        meshIndexBuffer.ImmutableStorage(nullptr,4,0);
        //meshShadowIndexBuffer.ImmutableStorage(nullptr, 4 , 0);
    }
    ~CommonBuffers() {}

    std::unordered_map<u32,MeshInternal> meshHandleTable;

    BufferObject<GL_ARRAY_BUFFER> meshVertexBuffer;
    //BufferObject<GL_ARRAY_BUFFER> meshShadowVertexBuffer;
    
    BufferObject<GL_ELEMENT_ARRAY_BUFFER> meshIndexBuffer;
    //BufferObject<GL_ELEMENT_ARRAY_BUFFER> meshShadowIndexBuffer;
    PersistentlyMappedBuffer<GL_COPY_READ_BUFFER> pinned;

    u32 meshID = 0;
    bool vaosDirty = false;
    u32 dirtyVaoCount = 0;
    
    std::mutex lck;
};

struct RenderState {
    alignas(64) CommonShaderParameteres shaderParams;
    RendererSettingsState* settings;
    CommonBuffers* buffers;

    BufferObject<GL_DRAW_INDIRECT_BUFFER> cmdBuffer;
    BufferObject<GL_ARRAY_BUFFER> matrixBuffer;
    PersistentlyMappedBuffer<GL_COPY_READ_BUFFER> pinned;
    VertexArray vaos[VAO_COUNT];

    u32 byteOffset;
    u32 cmdCount;
    u32 lineCount;
    u32 lineOffset;
    
    bool shaderParamDirtyFlag;
};

introspect(arg) struct RendererSettingsState {

    void* client;
    void (*OnCursorPos)(void* client , double x , double y);
    void* (*AllocateAligned)(void* client,u32 alignment,u32 size);
    void* (*Allocate)(void* client,u32 size);
    void (*Free)(void* client,void* mem);
    void (*NotifyClientWindowClose)(void* client);
    void (*OnResize)(void* client , u32 x , u32 y);
    const char* windowName;

    KeyCallBack* callbacks;
    i32 callbackCount;

    u32 windowWidth;
    u32 windowHeight;
};

struct Renderer {
    RendererSettingsState settings;
    RenderState* state;
    RenderState* newState;
    GLFWwindow* renderContext;
    GLFWwindow* mainContext;
    void* stackBase;
    void* stackPtr;

    u64* textureHandleTable;
    Vector<byte> tokenByteStream;

    CommonBuffers buffers;
    BufferObject<GL_UNIFORM_BUFFER> paramUbo;
    BufferObject<GL_UNIFORM_BUFFER> textureAddresses;

    VertexLayoutDescriptor descriptors[DESCRIPTOR_COUNT];
    Shader shaders[SHADER_COUNT];
    BindlessTexture textures[TEXTURE_COUNT];

    u32 renderStateRefCount;
    i32 toalAvailableMemory;
    i32 dedicatedVram;
    u32 currentVramENUM;
    
    bool stateUpdate;
    bool stoped;
    bool shouldRender;
    std::thread renderThread;
    std::mutex renderLck;
    std::mutex stateLck;
    std::mutex tokenStreamLck;
    std::mutex textureHandleLck;
};