#pragma once
#include <iostream>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <map>

#include <Context.h>
#include <BufferObject.h>
#include <TextureObject.h>
#include <VertexArray.h>
#include <Shader.h>
#include <Fonts.h>

struct DrawElementsIndirectCommand {
    uint count;
    uint instanceCount;
    uint firstIndex;
    uint baseVertex;
    uint baseInstance;
};

template<const GLenum bufferType> class PersistentlyMappedBuffer {
    public:
        PersistentlyMappedBuffer(const unsigned int size) {
            const GLenum storage = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
            const GLenum map = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
            ptr = buffer.ImmutableMap(nullptr , size , storage , map );
        }
        void ResizeMap(const unsigned int size) {
            const GLenum storage = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
            const GLenum map = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
            ptr = buffer.ImmutableResizeMap(nullptr , size , storage , map );
        }
        void Swap(PersistentlyMappedBuffer<bufferType>& other ) {
            std::swap(ptr , other.ptr);
            buffer.Swap( other.buffer );
        }
        const BufferObject<bufferType>& GetBuffer() const {
            return buffer;
        }

        BufferObject<bufferType> buffer;
        void* ptr = nullptr;
};

struct CommonShaderParams {
    glm::mat4 projectionViewMatrix;
    glm::mat4 inverseProjectionViewMatrix;
    glm::vec4 viewDir;
    glm::vec4 viewPos;
    glm::vec4 resolutionInverseResolution;
    float TIME = 0;
};
struct TextureHandleStream {
    std::mutex streamLck;
    std::vector<std::pair<uint64_t , uint32_t>> stream;
    uint16_t nonResidentOffset = 0;
    uint16_t count = 0;
};
struct alignas(64) RenderCommon {
    std::mutex renderLck;
    TextureHandleStream stream;
    bool cmdReadIndex = 1;
    bool shouldResize = false;
};
struct alignas(64) MainCommon {
    std::map<uint64_t , uint32_t> textureTranslationTable;
    bool cmdWriteIndex = 0;
    bool locked = false;
};
struct alignas(64) Common {
    std::mutex commonShaderParamsLck;
    std::mutex cmdBufferLck[2];
    std::mutex stateLck;
    CommonShaderParams commonShaderParams;
    std::atomic<bool> cmdBufferUpdate;
};
struct alignas(64) ReadOnly {
    ReadOnly(uint s1 , uint s2 , uint s3) : commonShaderParamUbo(s1) , commonTextureAddresses(s2) , cmdBuffer{s3,s3} {}
    PersistentlyMappedBuffer<GL_UNIFORM_BUFFER> commonShaderParamUbo;
    PersistentlyMappedBuffer<GL_UNIFORM_BUFFER> commonTextureAddresses;
    PersistentlyMappedBuffer<GL_DRAW_INDIRECT_BUFFER> cmdBuffer[2];
};
struct Mesh {
    uint32_t vertOffset;
    uint32_t vertSize;

    uint32_t indOffset;
    uint32_t indSize;

    uint16_t vertOffsetS;
    uint16_t vertSizeS;
    uint16_t indOffsetS;
    uint16_t indSizeS;
};

class Renderer;
class Global {
    public:
        Global(Renderer* r);
        ~Global();

        PersistentlyMappedBuffer<GL_DRAW_INDIRECT_BUFFER>& GetCmdBuffer();
        void BeginClient();
        void EndClient();

        void BeginDrawServer();
        void EndDrawServer();
        void InitServer();
        void CleanUpServer();

        Renderer* renderer;
        ReadOnly r;
        Common c;
        MainCommon m;
        RenderCommon rc;
};

class RenderPass {
    public:
        ~RenderPass();
        void BeginClient();
        void SubmitClient();
        void EndClient();

        void BeginDrawServer();
        void EndDrawServer();
    protected:

        RenderPass(Global* g);
        std::mutex bufferLck[2];
        Global* global;
        std::atomic<bool> finalBufferUpdate;
        bool bufferUpdate = 0;
        bool writeIndex = 0;
        bool readIndex = 1;
    private:
};

class RenderParam {
    public:
        RenderParam();
        uint16_t cmdOffset = 0;
        uint16_t cmdCount = 0;

        void Swap(RenderParam& other);
        void Init();
        void cleanUp();
        VertexArray& GetVao();
    private:
        uint64_t vao;
};

struct TextRenderState {
    TextRenderState();
    PersistentlyMappedBuffer<GL_ARRAY_BUFFER> vertexBuffer;
    PersistentlyMappedBuffer<GL_ARRAY_BUFFER> instancedBuffer;
    RenderParam param2d;
    RenderParam param3d;
    bool vaoInit = false;
};


class TextPass : public RenderPass {
    public:
        TextPass(Global* g);
        ~TextPass();
        
        TextRenderState& GetState();
    
        void HandleUpdatesServer();

        void ResizeServer(int w , int h);
        void DrawServer();
        void InitServer();
        void CleanUpServer();

        VertexArrayLayoutDescriptor layoutVbo;
        VertexArrayLayoutDescriptor layoutInstVbo;
        Shader shader;

        BufferObject<GL_ELEMENT_ARRAY_BUFFER> ibo;
        TextRenderState states[2];
};

struct SimpleRenderState {
    SimpleRenderState();
    RenderParam coloredParam;
    RenderParam texturedParam;
    PersistentlyMappedBuffer<GL_ARRAY_BUFFER> vbo;
    bool vaoInit = false;
};

class SimpleRasterPass : public RenderPass {
    public:
        SimpleRasterPass(Global* g);
        ~SimpleRasterPass();

        SimpleRenderState& GetState();

        void ResizeServer(int w , int h);
        void InitServer();
        void DrawServer();
        void CleanUpServer();

        void HandleUpdatesServer();

        Shader shader;
        VertexArrayLayoutDescriptor coloredVertexLayout;
        VertexArrayLayoutDescriptor texturedVertexLayout;
        BufferObject<GL_ELEMENT_ARRAY_BUFFER> ibo;
  
        SimpleRenderState states[2];
};


struct LightCullBuffer {
    BindlessTexture atomicLightCount;
    BufferObject<GL_SHADER_STORAGE_BUFFER> lightBuffer;
    BufferObject<GL_SHADER_STORAGE_BUFFER> computeDispatchBuffer;
    BufferObject<GL_ATOMIC_COUNTER_BUFFER> atomicCounter;
};

struct ShadowCmd {
    glm::vec4 posScale;
    uint mask;
    uint16_t size;
    uint16_t offset;
};
struct TiledState {
    TiledState();
    RenderParam Gvao;
    RenderParam lightVao;
    RenderParam shadowVao;
    PersistentlyMappedBuffer<GL_ARRAY_BUFFER> matrixLightBuffer;
    ShadowCmd cmd[32];
    bool vaoInit = false;
};

class GeometryPass : public RenderPass {
    public:
        GeometryPass(Global* g);
        ~GeometryPass();

        void InitServer();
        void DrawServer();
        void CleanUpServer();

        TiledState& GetState();
        void HandleServerUpdates();

        std::mutex meshLck;
        BufferObject<GL_ARRAY_BUFFER> meshVertexBuffer;
        BufferObject<GL_ELEMENT_ARRAY_BUFFER> meshIndexBuffer;
        BufferObject<GL_ARRAY_BUFFER> shadowVertexMesh;
        BufferObject<GL_ELEMENT_ARRAY_BUFFER> shadowIndexMesh;

        
        TiledState states[2];

        LightCullBuffer lightBuffer;
        uint Gbuffer;
        BindlessTexture shadowBuffer;
        BindlessTexture depth;
        BindlessTexture albedoRoughness;
        BindlessTexture normalMetallicAo;
        BindlessTexture computeOutput;
        Shader lightRasterPreShader;
        Shader lightRasterPostShader;
        Shader lightCullComputeShader;
        Shader computeClear;
        Shader tiledLightComputeShader;
        Shader geometryShader;
        Shader shadowBitTransferShader;
        Shader shadowShader;
        VertexArrayLayoutDescriptor meshLayout;
        VertexArrayLayoutDescriptor matrixLayout;
        VertexArrayLayoutDescriptor lightCullVertLayout;
        VertexArrayLayoutDescriptor lightCullInstLayout;
        VertexArrayLayoutDescriptor shadodwMatrixLayout;

        Shader debugShader;

        Mesh sphere;
        void* Iptr;
};

struct RendererArgs {
    const char* windowName;
    uint16_t height;
    uint16_t width;
    uint8_t syncInterval;
    uint8_t flags;
    uint32_t drawcallCount;
    uint32_t triangleCount;
    uint32_t vertexCount;
    uint32_t indexCount;
    void* user;
    void (*MousePosCallBack)(void* user,double x ,double y);
    void (*OnClose)(void* user);
    void (*OnResize)(void* user , int w , int h);
    void (*OnText)(void* user , uint codePoint);
    void (*OnModKey)(void* user , int key, int scancode, int action, int mods);
};


struct Texture {
    uint8_t mem[4];
};


class ResourceAllocator;

class ResourceBase {
    public:
    protected:
        virtual ~ResourceBase() = default;
    private:
        ResourceBase() = default;

        ResourceAllocator* const manager;
        std::string file;
        uint32_t refCount = 0;
        bool isLoaded = false;
};
class TextureResource : public ResourceBase {
    public:
        ~TextureResource() override {}
        uint64_t address;
        BindlessTexture texture;
};
class PBRTextureResource : public ResourceBase {
    public:
        ~PBRTextureResource() override {}
        uint64_t address;
        BindlessTexture texture;
};
class FontResource : public ResourceBase {
    public:
        ~FontResource() override {}
        uint64_t address;
        Font& GetFont();
        BindlessTexture& GetTexture();
    private:
        int32_t x,y = 0; 
        uint8_t mem[ sizeof(BindlessTexture) + sizeof(Font) ];
};

class MeshResource : public ResourceBase {
    public:
        ~MeshResource() override {}
        Mesh mesh;
};

typedef MeshResource* (*mesh_resource_de_ref_t)(void*);
typedef TextureResource* (*texture_resource_de_ref_t)(void*);
typedef PBRTextureResource* (*pbr_texture_resource_de_ref_t)(void*);
typedef FontResource* (*font_resource_de_ref_t)(void*);

template<class R> class SharedResourcePtr {
    public:
        static R* (*DeRef)( void* ptr );
        R* operator ->() {
            return DeRef( (void*)this );
        }
        R& operator *() {
            return *operator->();
        }
        R* Raw() {
            return m_resource;
        }
        bool IsNull() {
            return m_resource == nullptr;
        }
    private:
        R* m_resource;
};

class ResourceAllocator {
    public:
        void* renderer;
        void (*InitTexture)(Texture* texture);
        void (*FreeTexture)(Texture* texture);
        void (*Storage)(Texture* Texture , uint32_t internalFormat , uint32_t mipLevels, uint32_t x,uint32_t y,uint32_t z);
        void (*Upload)(Texture* texture , void* data , uint32_t dataFormat , uint32_t dataType , uint32_t mipLevel , uint32_t x,uint32_t y,uint32_t z);
        void (*SetTextureParamateres)(Texture* texture , uint32_t pname[] , const int32_t params[] , uint32_t size);
        uint64_t (*GetAddress)(Texture* texture);

        void(*RegisterTexture)(void* renderer , uint64_t address);
        void(*UnRegisterTexture)(void* renderer , uint64_t address);
    private:
        int32_t currentID = 0;
        
        template<typename> friend class SharedResourcePtr;

        std::unordered_map<int32_t, SharedResourcePtr<ResourceBase> > resources;
        std::mutex lck;
};
struct TextBox {
    std::string text;
    SharedResourcePtr<FontResource> fontPtr;
    glm::vec4 bounds;
    uint32_t nonSpaceCount;
    glm::vec<4 , uint8_t> color;
};
struct Text3D {
    std::string text;
    SharedResourcePtr<FontResource> fontPtr;
    glm::vec4 bounds;
    uint32_t nonSpaceCount;
};
struct PackedTintedTexturedAAQuad {
    SharedResourcePtr<TextureResource> ptr;
    glm::vec4 bound;
    glm::vec<4 , uint8_t> col[4];
};
struct PackedAAQuad {
    glm::vec4 bound;
    glm::vec<4 , uint8_t> col;
};
struct Renderable {
    SharedResourcePtr<MeshResource> mesh;
    SharedResourcePtr<PBRTextureResource> tex;
    glm::vec3 pos;
    glm::vec3 rot;
    glm::vec3 scale;
};
struct ClientLight {
    glm::vec4 posScale;
    glm::vec3 col;
    bool shadow;
};

struct ClientRendererState {
    std::unordered_set<uint64_t> unique;
    DrawElementsIndirectCommand* cmd;

    ClientLight* lights;
    TextBox* array2d;
    Text3D* array3d;
    PackedAAQuad* coloredAAquads;
    Renderable* meshes;
    PackedTintedTexturedAAQuad* texturedAAQuads;

    uint32_t charCount2d;
    uint32_t charCount3d;

    uint16_t offset;
    uint16_t size2d;
    uint16_t size3d;
    uint16_t ucount2d;
    uint16_t ucount3d;
    uint16_t coloredAAQuadSize;
    uint16_t texturedAAQuadSize;
    uint16_t meshSize;
    uint16_t uMeshes;
    uint16_t lightCount;
};
struct Renderer {
    RendererArgs* args;
    Context main;
    Context render;

    Global global;
    GeometryPass gPass;
    TextPass text;
    SimpleRasterPass raster;
    ClientRendererState frontEnd;

    std::thread renderThread;
    bool shouldClose = false;
};

void RenderLoop(Renderer* r);
void MakeAAQuad(float*& write, glm::vec4 quad);
void MakeUniformlyColoredAAQuad(void*& write, const PackedAAQuad& quad);
void MakeTexturedAAQuad(void*& write , const PackedTintedTexturedAAQuad& quad , const uint32_t index);
void MakeTextQuads( TextBox& text ,  Quad*& write);
void MakeTextQuads2( Text3D& text ,  Quad*& write);