#include <glm/gtx/euler_angles.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/scalar_multiplication.hpp>
#include <OBJ_Loader.h>

#include <InterFaceFunctions.h>
#include <UtilityFuncs.h>

void ModKeyCallBack(GLFWwindow* window , int key, int scancode, int action, int mods) {
    auto renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
    renderer->args->OnModKey(renderer->args->user , key , scancode , action , mods);
}
void TextCallBack(GLFWwindow* window , uint codePoint) {
    auto renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
    renderer->args->OnText( renderer->args->user , codePoint );
}
void MouseCallBack(GLFWwindow* w, double x ,double y) {
    GLFWCall(Renderer* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(w)));
    renderer->args->MousePosCallBack( renderer->args->user , x ,y );
}
void WindowCloseCallback(GLFWwindow* window) {
    auto renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
    renderer->args->OnClose( renderer->args->user );
}
void WindowResizeCallBack(GLFWwindow* window , int w , int h) {
    auto renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
    renderer->args->OnResize( renderer->args->user , w , h );
}

template<> font_resource_de_ref_t SharedResourcePtr<FontResource>::DeRef = nullptr;
template<> texture_resource_de_ref_t SharedResourcePtr<TextureResource>::DeRef = nullptr;
template<> pbr_texture_resource_de_ref_t SharedResourcePtr<PBRTextureResource>::DeRef = nullptr;
template<> mesh_resource_de_ref_t SharedResourcePtr<MeshResource>::DeRef = nullptr;

Font& FontResource::GetFont() {
            return *((Font*)mem);
}
BindlessTexture& FontResource::GetTexture() {
    return *((BindlessTexture*)(mem + sizeof(Font)));
}

RenderParam::RenderParam() : cmdOffset(0) , cmdCount(0) {}
void RenderParam::Swap(RenderParam& other) {
    std::swap(vao , other.vao);
    std::swap(cmdCount , other.cmdCount);
    std::swap(cmdOffset , other.cmdOffset);
}
void RenderParam::Init() {
    new(&vao) VertexArray;
}
void RenderParam::cleanUp() {
    ((VertexArray*)&vao)->~VertexArray();
}
VertexArray& RenderParam::GetVao() {
    return *((VertexArray*)&vao);
}

Global::Global(Renderer* r) : renderer(r) , r(sizeof(CommonShaderParams),240,200) {}

Global::~Global() {}

PersistentlyMappedBuffer<GL_DRAW_INDIRECT_BUFFER>& Global::GetCmdBuffer() {
    return r.cmdBuffer[m.cmdWriteIndex];
}
void Global::BeginClient() {
    c.cmdBufferUpdate = false;
    if( c.cmdBufferLck[!rc.cmdReadIndex].try_lock() ) {
        m.cmdWriteIndex = !rc.cmdReadIndex;
    }
    else if( c.cmdBufferLck[rc.cmdReadIndex].try_lock() ) {
        m.cmdWriteIndex = rc.cmdReadIndex;
    }
    else {
        LOGASSERT(false , "Invalid Rendering State");
    }
}
void Global::EndClient() {
    GLCall(glFinish());
    c.cmdBufferUpdate = true;
    c.cmdBufferLck[m.cmdWriteIndex].unlock();
}
void Global::BeginDrawServer() {

    if( rc.shouldResize ) {
        rc.shouldResize = 0;
    }
    
    if( c.commonShaderParamsLck.try_lock() ) {
        *((CommonShaderParams*)r.commonShaderParamUbo.ptr) = c.commonShaderParams;
        c.commonShaderParamsLck.unlock();
    }

    //------------------------

    rc.stream.streamLck.lock();
    for(int i = 0; i < rc.stream.nonResidentOffset ; i++) {
        GLCall(glMakeTextureHandleResidentARB( rc.stream.stream[i].first ));
        ASSERT( glIsTextureHandleResidentARB(rc.stream.stream[i].first) );
        ((uint64_t*)r.commonTextureAddresses.ptr)[ rc.stream.stream[i].second ] = rc.stream.stream[i].first;
    }
    for(int i = rc.stream.nonResidentOffset; i < rc.stream.stream.size() ; i++) {
        GLCall(glMakeTextureHandleNonResidentARB( rc.stream.stream[i].first ));
        ASSERT( !glIsTextureHandleResidentARB(rc.stream.stream[i].first) );
    }
    rc.stream.stream.clear();
    rc.stream.nonResidentOffset = 0;
    rc.stream.streamLck.unlock();

    c.stateLck.lock();
    if( c.cmdBufferUpdate ) {
        c.cmdBufferUpdate = false;
        rc.cmdReadIndex = !rc.cmdReadIndex;
    }
    c.cmdBufferLck[rc.cmdReadIndex].lock();

    r.cmdBuffer[rc.cmdReadIndex].GetBuffer().Bind();
}
void Global::EndDrawServer() {
    GLCall(glFinish());
    c.stateLck.lock();
    c.cmdBufferLck[rc.cmdReadIndex].unlock();
}
void Global::InitServer() {
    GLCall(glClearColor(0,0,0,0));
    r.commonShaderParamUbo.GetBuffer().BindBase(0);
    r.commonTextureAddresses.GetBuffer().BindBase(1);
}
void Global::CleanUpServer() {

    rc.stream.streamLck.lock();
    for(const auto& v : m.textureTranslationTable) {
        ASSERT( glIsTextureHandleResidentARB(v.first) );
        GLCall(glMakeTextureHandleNonResidentARB( v.first ));
        ASSERT( !glIsTextureHandleResidentARB(v.first) );
    }
    rc.stream.streamLck.unlock();
}

RenderPass::RenderPass(Global* g) : finalBufferUpdate(false) , global(g) {}
RenderPass::~RenderPass() {}

void RenderPass::BeginClient() {
    bufferUpdate = false;
    finalBufferUpdate = false;
    if( bufferLck[!readIndex].try_lock() ) {
        writeIndex = !readIndex;
    }
    else if( bufferLck[readIndex].try_lock() ) {
        writeIndex = readIndex;
    }
    else {
        LOGASSERT(false , "Invalid rendering state");
    }
}
void RenderPass::SubmitClient() {
    bufferUpdate = true;
}
void RenderPass::EndClient() {
    if( bufferUpdate ) {
        finalBufferUpdate = true;
    }
    bufferLck[writeIndex].unlock();
}

void RenderPass::BeginDrawServer() {
    if( finalBufferUpdate ) {
        finalBufferUpdate = false;
        readIndex = !readIndex;
    }
    bufferLck[readIndex].lock();
}
void RenderPass::EndDrawServer() {
    bufferLck[readIndex].unlock();
}

TextRenderState::TextRenderState() : vertexBuffer(1) , instancedBuffer(1) {}
TextPass::~TextPass() {}
TextPass::TextPass(Global* g) : RenderPass(g) {
    shader.MakeShader("../res/shaders/DistanceFieldTextShader.glsl");
    
    ibo.ResizeBuffer(1);

    layoutVbo.Begin();
    layoutVbo.Push(GL_FLOAT , GL_FLOAT, 2);
    layoutVbo.Push(GL_FLOAT , GL_FLOAT, 2);

    layoutInstVbo.Begin();
    layoutInstVbo.Push(GL_UNSIGNED_INT , GL_INT , 1 , 1);
}
TextRenderState& TextPass::GetState() {
    return states[writeIndex];
}
void TextPass::ResizeServer(int w , int h) {}
void TextPass::InitServer() {
    states[0].param2d.Init();
    states[0].param2d.GetVao().AddBuffer( states[0].vertexBuffer.GetBuffer() , layoutVbo );
    states[0].param2d.GetVao().AddBuffer( states[0].instancedBuffer.GetBuffer() , layoutInstVbo );
    states[0].param2d.GetVao().AddBuffer( ibo );
    states[0].vaoInit = false;

    states[1].param2d.Init();
    states[1].param2d.GetVao().AddBuffer( states[1].vertexBuffer.GetBuffer() , layoutVbo );
    states[1].param2d.GetVao().AddBuffer( states[1].instancedBuffer.GetBuffer() , layoutInstVbo );
    states[1].param2d.GetVao().AddBuffer( ibo );
    states[1].vaoInit = false;
}
void TextPass::CleanUpServer() {
    states[0].param2d.cleanUp();
    states[1].param2d.cleanUp();
}
void TextPass::HandleUpdatesServer() {
    auto& state = states[readIndex];

    if( state.vaoInit ) {
        state.vaoInit = false;
        auto& vao = state.param2d.GetVao();
        auto& iVbo = state.instancedBuffer.GetBuffer();
        auto& vbo = state.vertexBuffer.GetBuffer();

        vao.NullIndex();
        vao.AddBuffer( vbo , layoutVbo );
        vao.AddBuffer( iVbo , layoutInstVbo );
        vao.AddBuffer( ibo );
    }
}
void TextPass::DrawServer() {

    HandleUpdatesServer();

    GLCall(glEnable(GL_BLEND));
    GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
    
    auto& state = states[readIndex];

    shader.Bind();
    uint32_t indices[1]{0};
    GLCall(glUniformSubroutinesuiv( GL_VERTEX_SHADER , 1 , indices));
    state.param2d.GetVao().Bind();
    //GLCall(glMultiDrawElementsIndirect(GL_TRIANGLES , GL_UNSIGNED_INT , (void*)(state.param2d.cmdOffset * sizeof(DrawElementsIndirectCommand)) , state.param2d.cmdCount , 0 ));
    
    indices[0] = 1;
    GLCall(glUniformSubroutinesuiv( GL_VERTEX_SHADER , 1 , indices));
    //GLCall(glMultiDrawElementsIndirect(GL_TRIANGLES , GL_UNSIGNED_INT , (void*)(state.param3d.cmdOffset * sizeof(DrawElementsIndirectCommand)) , state.param3d.cmdCount , 0 ));
}

SimpleRenderState::SimpleRenderState() : vbo(1) {}
SimpleRasterPass::~SimpleRasterPass() {}
SimpleRasterPass::SimpleRasterPass(Global* g) : RenderPass(g) {
    shader.MakeShader( "../res/shaders/SimpleShader.glsl" );

    ibo.ResizeBuffer(1);

    coloredVertexLayout.Begin();
    coloredVertexLayout.Push(GL_FLOAT , GL_FLOAT , 2);
    coloredVertexLayout.Push(GL_INT , GL_INT , 1);

    texturedVertexLayout.Begin();
    texturedVertexLayout.Push(GL_FLOAT , GL_FLOAT , 2);
    texturedVertexLayout.Push(GL_INT , GL_INT , 1);
    texturedVertexLayout.Push(GL_UNSIGNED_INT , GL_INT , 1);
}
SimpleRenderState& SimpleRasterPass::GetState() {
    return states[writeIndex];
}
void SimpleRasterPass::ResizeServer(int w , int h) {}
void SimpleRasterPass::InitServer() {
    states[0].coloredParam.Init();
    states[0].coloredParam.GetVao().AddBuffer( states[0].vbo.GetBuffer() , coloredVertexLayout );
    states[0].coloredParam.GetVao().AddBuffer( ibo );

    states[0].texturedParam.Init();
    states[0].texturedParam.GetVao().AddBuffer( states[0].vbo.GetBuffer() , texturedVertexLayout );
    states[0].texturedParam.GetVao().AddBuffer( ibo );
    states[0].vaoInit = false;

    states[1].coloredParam.Init();
    states[1].coloredParam.GetVao().AddBuffer( states[1].vbo.GetBuffer() , coloredVertexLayout );
    states[1].coloredParam.GetVao().AddBuffer( ibo );

    states[1].texturedParam.Init();
    states[1].texturedParam.GetVao().AddBuffer( states[1].vbo.GetBuffer() , texturedVertexLayout );
    states[1].texturedParam.GetVao().AddBuffer( ibo );
    states[1].vaoInit = false;
}
void SimpleRasterPass::CleanUpServer() {
    states[0].coloredParam.cleanUp();
    states[0].texturedParam.cleanUp();

    states[1].coloredParam.cleanUp();
    states[1].texturedParam.cleanUp();
}
void SimpleRasterPass::HandleUpdatesServer() {
    if( states[readIndex].vaoInit ) {
        states[readIndex].vaoInit = false;

        auto& coloredVao = states[readIndex].coloredParam.GetVao();
        auto& texturedVao = states[readIndex].texturedParam.GetVao();
        auto& vbo = states[readIndex].vbo.GetBuffer();

        coloredVao.NullIndex();
        coloredVao.AddBuffer( vbo , coloredVertexLayout );
        coloredVao.AddBuffer( ibo );


        texturedVao.NullIndex();
        texturedVao.AddBuffer( vbo , texturedVertexLayout );
        texturedVao.AddBuffer( ibo );
    }
}
void SimpleRasterPass::DrawServer() {
    
    HandleUpdatesServer();
    GLCall(glBindFramebuffer(GL_FRAMEBUFFER,0));
    GLCall(glDepthMask(GL_FALSE));
    GLCall(glDisable(GL_CULL_FACE));
    GLCall(glDisable(GL_DEPTH_TEST));
    GLCall(glDisable(GL_STENCIL_TEST));
    GLCall(glColorMask(GL_TRUE , GL_TRUE , GL_TRUE , GL_TRUE));
    GLCall(glClear(GL_COLOR_BUFFER_BIT));

    auto& state = states[readIndex];
    shader.Bind();

    GLuint indices[1]{0};
    GLCall(glUniformSubroutinesuiv( GL_VERTEX_SHADER , 1 , indices));
    GLCall(glUniformSubroutinesuiv( GL_FRAGMENT_SHADER , 1 , indices));
    state.coloredParam.GetVao().Bind();
    GLCall(glMultiDrawElementsIndirect(GL_TRIANGLES , GL_UNSIGNED_INT , (void*)(state.coloredParam.cmdOffset * sizeof(DrawElementsIndirectCommand)) , state.coloredParam.cmdCount , 0 ));


    indices[0] = 1;
    GLCall(glUniformSubroutinesuiv( GL_VERTEX_SHADER , 1 , indices));
    GLCall(glUniformSubroutinesuiv( GL_FRAGMENT_SHADER , 1 , indices));
    state.texturedParam.GetVao().Bind();
    GLCall(glMultiDrawElementsIndirect(GL_TRIANGLES , GL_UNSIGNED_INT , (void*)(state.texturedParam.cmdOffset * sizeof(DrawElementsIndirectCommand)) , state.texturedParam.cmdCount , 0 ));
}

constexpr float cube[36 * 3] = {
        -1.0f,-1.0f,-1.0f,
        -1.0f,-1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,
        1.0f, 1.0f,-1.0f,
        -1.0f,-1.0f,-1.0f,
        -1.0f, 1.0f,-1.0f,
        1.0f,-1.0f, 1.0f,
        -1.0f,-1.0f,-1.0f,
        1.0f,-1.0f,-1.0f,
        1.0f, 1.0f,-1.0f,
        1.0f,-1.0f,-1.0f,
        -1.0f,-1.0f,-1.0f,
        -1.0f,-1.0f,-1.0f,
        -1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f,-1.0f,
        1.0f,-1.0f, 1.0f,
        -1.0f,-1.0f, 1.0f,
        -1.0f,-1.0f,-1.0f,
        -1.0f, 1.0f, 1.0f,
        -1.0f,-1.0f, 1.0f,
        1.0f,-1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f,-1.0f,-1.0f,
        1.0f, 1.0f,-1.0f,
        1.0f,-1.0f,-1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f,-1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f,-1.0f,
        -1.0f, 1.0f,-1.0f,
        1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f,-1.0f,
        -1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,
        1.0f,-1.0f, 1.0f,
};

TiledState::TiledState() : matrixLightBuffer(1) {}

struct I {
    glm::vec4 p;
    glm::vec3 c;
    uint shadowMask;
};
GeometryPass::~GeometryPass() {}
GeometryPass::GeometryPass(Global* g) : RenderPass(g) {


    meshVertexBuffer.Bind();
    GLCall(glBufferStorage(GL_ARRAY_BUFFER , 32 , nullptr , GL_DYNAMIC_STORAGE_BIT));
    meshIndexBuffer.Bind();
    GLCall(glBufferStorage(GL_ELEMENT_ARRAY_BUFFER , 4 , nullptr , GL_DYNAMIC_STORAGE_BIT));
    shadowVertexMesh.Bind();
    GLCall(glBufferStorage(GL_ARRAY_BUFFER , 12 , nullptr , GL_DYNAMIC_STORAGE_BIT));
    shadowIndexMesh.Bind();
    GLCall(glBufferStorage(GL_ELEMENT_ARRAY_BUFFER , 4 , nullptr , GL_DYNAMIC_STORAGE_BIT));


    objl::Loader loader;
    loader.LoadFile("../res/models/untitled.obj");
    std::vector<glm::vec3> pos;
    for(int i = 0; i < loader.LoadedVertices.size() ; i++) {
        pos.emplace_back( loader.LoadedVertices[i].Position.X,loader.LoadedVertices[i].Position.Y,loader.LoadedVertices[i].Position.Z );
        pos.back() = glm::normalize(pos.back());
    }
    lightVolumeMesh = RegisterMesh(global->renderer , &pos[0] , pos.size() * sizeof(glm::vec3) , &loader.LoadedIndices[0] , loader.LoadedIndices.size() * 4 , false );

    meshLayout.Begin();
    meshLayout.Push(GL_FLOAT , GL_FLOAT , 3,0);
    meshLayout.Push(GL_FLOAT , GL_FLOAT , 3,0);
    meshLayout.Push(GL_FLOAT , GL_FLOAT , 2,0);

    matrixLayout.Begin();
    matrixLayout.Push(GL_FLOAT , GL_FLOAT , 3 , 1);
    matrixLayout.Push(GL_FLOAT , GL_FLOAT , 3 , 1);
    matrixLayout.Push(GL_FLOAT , GL_FLOAT , 3 , 1);
    matrixLayout.Push(GL_FLOAT , GL_FLOAT , 3 , 1);
    matrixLayout.Push(GL_UNSIGNED_INT , GL_INT , 1 , 1);

    lightCullVertLayout.Begin();
    lightCullVertLayout.Push(GL_FLOAT , GL_FLOAT , 3 , 0);

    lightCullInstLayout.Begin();
    lightCullInstLayout.Push(GL_FLOAT , GL_FLOAT , 4 , 1);
    lightCullInstLayout.Push(GL_FLOAT , GL_FLOAT , 4 , 1);

    shadowMatrixLayout.Begin();
    shadowMatrixLayout.Push(GL_FLOAT , GL_FLOAT , 3 , 1);
    shadowMatrixLayout.Push(GL_FLOAT , GL_FLOAT , 3 , 1);
    shadowMatrixLayout.Push(GL_FLOAT , GL_FLOAT , 3 , 1);
    shadowMatrixLayout.Push(GL_FLOAT , GL_FLOAT , 3 , 1);

    shadowVertexRecordLayout.Begin();
    shadowVertexRecordLayout.Push(GL_FLOAT , GL_FLOAT ,4);

    geometryShader.MakeShader("../res/shaders/GeometryShader.glsl");
    tiledLightComputeShader.MakeShader("../res/shaders/TiledLightCompute.glsl");
    lightCullComputeShader.MakeShader("../res/shaders/LightCullCompute.glsl");
    lightRasterPreShader.MakeShader("../res/shaders/LightCullPrePassShader.glsl");
    lightRasterPostShader.MakeShader("../res/shaders/LightCullPostPassShader.glsl");
    const char* names[1] = {"shadowVertex"};
    shadowShader.MakeShader("../res/shaders/ShadowShader.glsl" , names , 0);
    shadowBitTransferShader.MakeShader("../res/shaders/ShadowBitTransferShader.glsl");
    fogComputeShader.MakeShader("../res/shaders/TileFogCompute.glsl");
    fogShadowShader.MakeShader("../res/shaders/VolumetricShadowIntegration.glsl");
    debugShader.MakeShader("../res/shaders/DebugShader.glsl");

    uint x = global->renderer->args->width;
    uint y = global->renderer->args->height;
    depth.Init();                                                           // slot 0
    depth.Bind<GL_TEXTURE_2D>();
    depth.ImmutableStorage<GL_TEXTURE_2D>(GL_DEPTH24_STENCIL8 , 1 , x,y,1);
    RegisterTexture( global->renderer ,depth.GetAddress());

    albedoRoughness.Init();                                                 // slot 1
    albedoRoughness.Bind<GL_TEXTURE_2D>();
    albedoRoughness.ImmutableStorage<GL_TEXTURE_2D>(GL_RGBA8 , 1 , x,y,1);
    RegisterTexture( global->renderer ,albedoRoughness.GetAddress());

    normalMetallicAo.Init();                                                // slot 2
    normalMetallicAo.Bind<GL_TEXTURE_2D>();
    normalMetallicAo.ImmutableStorage<GL_TEXTURE_2D>(GL_RGB16UI , 1 , x,y,1);
    RegisterTexture( global->renderer ,normalMetallicAo.GetAddress());

    computeOutput.Init();                                                   // slot 3
    computeOutput.Bind<GL_TEXTURE_2D>();
    computeOutput.ImmutableStorage<GL_TEXTURE_2D>(GL_RGBA8, 1 , 1920 , 1080 , 1 );
    RegisterTexture( global->renderer ,computeOutput.GetAddress());

    lightBuffer.atomicLightCount.Init();                                    // slot 4
    lightBuffer.atomicLightCount.Bind<GL_TEXTURE_2D>();
    lightBuffer.atomicLightCount.ImmutableStorage<GL_TEXTURE_2D>(GL_RGBA8 , 1, 240,135,1);
    RegisterTexture(global->renderer , lightBuffer.atomicLightCount.GetAddress());

    shadowBuffer.Init();                                                    // slot 5
    shadowBuffer.Bind<GL_TEXTURE_2D>();
    shadowBuffer.ImmutableStorage<GL_TEXTURE_2D>( GL_R32UI , 1 , 1920,1080,1 );
    RegisterTexture(global->renderer , shadowBuffer.GetAddress() );

    funclookupTable_64x64xhalf.Init();                                      // slot 6
    funclookupTable_64x64xhalf.Bind<GL_TEXTURE_2D>();
    funclookupTable_64x64xhalf.ImmutableStorage<GL_TEXTURE_2D>( GL_R16F , 1 , 64 ,64 , 1 );
    float tex[64*64]{0};
    MakeFuncLookupTable(tex , 64 , 1000.f);
    funclookupTable_64x64xhalf.Upload<GL_TEXTURE_2D>(tex , GL_RED , GL_FLOAT , 0 , 64,64,1);
    RegisterTexture(global->renderer , funclookupTable_64x64xhalf.GetAddress());

    lightBuffer.computeDispatchBuffer.Bind();
    GLCall(glBufferStorage(GL_SHADER_STORAGE_BUFFER , 240*135*4 , nullptr , GL_MAP_READ_BIT));
    lightBuffer.lightBuffer.Bind();
    GLCall(glBufferStorage(GL_SHADER_STORAGE_BUFFER , 240*135*(1000) , nullptr , GL_MAP_READ_BIT));
    lightBuffer.atomicCounter.Bind();
    GLCall(glBufferStorage(GL_ATOMIC_COUNTER_BUFFER , 16 , nullptr , GL_DYNAMIC_STORAGE_BIT));
}
void GeometryPass::InitServer() {
    GLCall(glGenFramebuffers(1 , &Gbuffer));
    GLCall(glBindFramebuffer(GL_FRAMEBUFFER, Gbuffer));
    GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER , GL_DEPTH_STENCIL_ATTACHMENT , GL_TEXTURE_2D,depth.ID ,0));
    GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER , GL_COLOR_ATTACHMENT0 , GL_TEXTURE_2D,albedoRoughness.ID ,0));
    GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER , GL_COLOR_ATTACHMENT1 , GL_TEXTURE_2D,normalMetallicAo.ID ,0));
    GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER , GL_COLOR_ATTACHMENT2 , GL_TEXTURE_2D,shadowBuffer.ID ,0));
    GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER , GL_COLOR_ATTACHMENT3 , GL_TEXTURE_2D,computeOutput.ID ,0));
    GLenum buff[4] = {GL_COLOR_ATTACHMENT0,GL_COLOR_ATTACHMENT1,GL_COLOR_ATTACHMENT2,GL_COLOR_ATTACHMENT3};
    GLCall(glDrawBuffers(4,buff));
 
    ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

    GLCall(glGenQueries(1,&query));
    GLCall(glGenTransformFeedbacks(1 , &tranformFeedbackState));
    GLCall(glBindTransformFeedback(GL_TRANSFORM_FEEDBACK , tranformFeedbackState));
    GLCall(glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER , shadowRecordBuffer.GetID() ))
    GLCall(glBufferStorage(GL_TRANSFORM_FEEDBACK_BUFFER , 5000000 , nullptr , GL_DYNAMIC_STORAGE_BIT));
    GLCall(glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER,0,shadowRecordBuffer.GetID()));
    
    states[0].Gvao.Init();
    states[0].Gvao.GetVao().AddBuffer( meshVertexBuffer , meshLayout );
    states[0].Gvao.GetVao().AddBuffer( states[0].matrixLightBuffer.GetBuffer() , matrixLayout );
    states[0].Gvao.GetVao().AddBuffer( meshIndexBuffer );

    states[0].lightVao.Init();
    states[0].lightVao.GetVao().AddBuffer(meshVertexBuffer , lightCullVertLayout);
    states[0].lightVao.GetVao().AddBuffer(states[0].matrixLightBuffer.GetBuffer() , lightCullInstLayout);
    states[0].lightVao.GetVao().AddBuffer(meshIndexBuffer);

    states[0].shadowVao.Init();
    states[0].shadowVao.GetVao().AddBuffer(shadowVertexMesh , lightCullVertLayout);
    states[0].shadowVao.GetVao().AddBuffer(states[0].matrixLightBuffer.GetBuffer() , shadowMatrixLayout);
    states[0].shadowVao.GetVao().AddBuffer(shadowIndexMesh);

    states[1].Gvao.Init();
    states[1].Gvao.GetVao().AddBuffer( meshVertexBuffer , meshLayout );
    states[1].Gvao.GetVao().AddBuffer( states[1].matrixLightBuffer.GetBuffer() , matrixLayout );
    states[1].Gvao.GetVao().AddBuffer( meshIndexBuffer );

    states[1].lightVao.Init();
    states[1].lightVao.GetVao().AddBuffer(meshVertexBuffer , lightCullVertLayout);
    states[1].lightVao.GetVao().AddBuffer(states[1].matrixLightBuffer.GetBuffer() , lightCullInstLayout);
    states[1].lightVao.GetVao().AddBuffer(meshIndexBuffer);

    states[1].shadowVao.Init();
    states[1].shadowVao.GetVao().AddBuffer(shadowVertexMesh , lightCullVertLayout);
    states[1].shadowVao.GetVao().AddBuffer(states[1].matrixLightBuffer.GetBuffer() , shadowMatrixLayout);
    states[1].shadowVao.GetVao().AddBuffer(shadowIndexMesh);

    new (volShadowVao) VertexArray;
    ((VertexArray*)volShadowVao)->AddBuffer( *((BufferObject<GL_ARRAY_BUFFER>*)&shadowRecordBuffer) ,shadowVertexRecordLayout );

    GLCall(glBindImageTexture(0, computeOutput.ID , 0 , GL_FALSE , 0 , GL_WRITE_ONLY , GL_RGBA8));
    GLCall(glBindImageTexture(1, lightBuffer.atomicLightCount.ID , 0 , GL_FALSE , 0 , GL_READ_WRITE , GL_R32UI));
    lightBuffer.lightBuffer.BindBase(2);
    lightBuffer.computeDispatchBuffer.BindBase(4);
    lightBuffer.atomicCounter.BindBase(5);
}
void GeometryPass::CleanUpServer() {
    ((VertexArray*)volShadowVao)->~VertexArray();
    states[0].Gvao.cleanUp();
    states[0].lightVao.cleanUp();
    states[1].Gvao.cleanUp();
    states[1].lightVao.cleanUp();
    UnRegisterTexture(global->renderer , shadowBuffer.GetAddress());
    UnRegisterTexture(global->renderer , lightBuffer.atomicLightCount.GetAddress());
    UnRegisterTexture(global->renderer , computeOutput.GetAddress());
    UnRegisterTexture(global->renderer , normalMetallicAo.GetAddress());
    UnRegisterTexture(global->renderer , albedoRoughness.GetAddress());
    UnRegisterTexture(global->renderer , depth.GetAddress());

    GLCall(glDeleteTransformFeedbacks(1, &tranformFeedbackState));
    GLCall(glDeleteFramebuffers(1, &Gbuffer));
}
TiledState& GeometryPass::GetState() {
    return states[writeIndex];
}
void GeometryPass::HandleServerUpdates() {
    auto& state = states[readIndex];
    if(state.vaoInit) {
        meshLck.lock();
        state.vaoInit = false;
        state.Gvao.GetVao().NullIndex();
        state.Gvao.GetVao().AddBuffer( meshVertexBuffer , meshLayout );
        state.Gvao.GetVao().AddBuffer( state.matrixLightBuffer.GetBuffer() , matrixLayout );
        state.Gvao.GetVao().AddBuffer( meshIndexBuffer );
        state.lightVao.GetVao().NullIndex();
        state.lightVao.GetVao().AddBuffer( meshVertexBuffer , lightCullVertLayout );
        state.lightVao.GetVao().AddBuffer( state.matrixLightBuffer.GetBuffer() , lightCullInstLayout );
        state.lightVao.GetVao().AddBuffer( meshIndexBuffer );
        state.shadowVao.GetVao().NullIndex();
        state.shadowVao.GetVao().AddBuffer(shadowVertexMesh, lightCullVertLayout );
        state.shadowVao.GetVao().AddBuffer(state.matrixLightBuffer.GetBuffer() , shadowMatrixLayout );
        state.shadowVao.GetVao().AddBuffer(shadowIndexMesh);
        meshLck.unlock();
    }
}


void GeometryPass::DrawServer() {

    HandleServerUpdates();
    GLCall(glBindFramebuffer(GL_FRAMEBUFFER,Gbuffer));
    GLCall(glViewport(0,0,1920,1080));
    
    GLCall(glDepthMask(GL_TRUE));
    GLCall(glEnable(GL_DEPTH_TEST));
    GLCall(glDisable(GL_STENCIL_TEST));
    GLCall(glDisable(GL_BLEND));
    GLCall(glEnable(GL_CULL_FACE));
    GLCall(glCullFace(GL_BACK));
    GLCall(glColorMask(GL_TRUE , GL_TRUE , GL_TRUE , GL_TRUE));
	GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT));

    auto& state = states[readIndex];

    geometryShader.Bind();
    state.Gvao.GetVao().Bind();
    GLCall(glMultiDrawElementsIndirect(GL_TRIANGLES , GL_UNSIGNED_INT , (void*)(state.Gvao.cmdOffset * sizeof(DrawElementsIndirectCommand)) , state.Gvao.cmdCount , 0));

    struct Light {
        glm::vec4 posScale;
        glm::vec3 color;
        uint mask;
    };

    GLCall(glEnable(GL_COLOR_LOGIC_OP));
    GLCall(glLogicOp(GL_OR));
    GLCall(glDepthMask(GL_FALSE));
    GLCall(glDisable(GL_DEPTH_TEST));
    GLCall(glDisable(GL_CULL_FACE));
    GLCall(glEnable(GL_STENCIL_TEST));
    GLCall(glEnable(GL_DEPTH_CLAMP));

    static bool s = false;
    for(int i = 0; i < std::min(state.shadowVao.cmdCount , (uint16_t)32); i++) {
        s = true;

        glm::vec4& p = state.cmd[i].posScale;

        GLCall(glEnable(GL_DEPTH_TEST));
        GLCall(glColorMask(GL_FALSE , GL_FALSE , GL_FALSE , GL_FALSE));

        GLCall(glStencilFunc(GL_ALWAYS, 0, 0xff));
        GLCall(glStencilOpSeparate(GL_BACK, GL_KEEP, GL_INCR_WRAP, GL_KEEP));
        GLCall(glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_DECR_WRAP, GL_KEEP));

        shadowShader.Bind();
        GLCall(glUniform4f(1,p.x,p.y,p.z,p.w));
        state.shadowVao.GetVao().Bind();

        // GLCall(glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, query));
        // GLCall(glBindTransformFeedback(GL_TRANSFORM_FEEDBACK , tranformFeedbackState));
        // GLCall(glBeginTransformFeedback(GL_TRIANGLES));
        GLCall(glMultiDrawElementsIndirect(GL_TRIANGLES_ADJACENCY , GL_UNSIGNED_INT , (void*)(state.cmd[i].offset * sizeof(DrawElementsIndirectCommand)) , state.cmd[i].size , 0));
        // GLCall(glEndTransformFeedback());
        // GLCall(glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN));
        // GLuint primitives;
        // GLCall(glGetQueryObjectuiv(query, GL_QUERY_RESULT, &primitives));
        // std::cout << primitives << std::endl;
    
        glm::vec4 b;
        GetLightAABB( p , global->c.commonShaderParams , b );
        glm::vec2 quad[4];
        quad[0] = {b.x,b.y};
        quad[1] = {b.z,b.y};
        quad[2] = {b.z,b.w};
        quad[3] = {b.x,b.w};

        GLCall(glDisable(GL_DEPTH_TEST));
        GLCall(glStencilFunc(GL_NOTEQUAL, 0, 0xff));
        GLCall(glStencilOp(GL_KEEP,GL_KEEP,GL_REPLACE));
        GLCall(glColorMask(GL_TRUE , GL_TRUE , GL_TRUE , GL_TRUE));
        
        shadowBitTransferShader.Bind();
        GLCall(glUniform2fv( 2 , 4 , (float*)quad ));
        GLCall(glUniform1ui(1, state.cmd[i].mask ));
        global->renderer->raster.GetState().coloredParam.GetVao().Bind();
        GLCall(glDrawElements(GL_TRIANGLES , 6 , GL_UNSIGNED_INT , nullptr));
    }

    GLCall(glDisable(GL_DEPTH_CLAMP));
    GLCall(glDisable(GL_COLOR_LOGIC_OP));

    uint clear = 0;
    GLCall(glClearTexImage(computeOutput.ID  ,0, GL_RGBA , GL_UNSIGNED_BYTE , &clear ));
    GLCall(glClearTexImage(lightBuffer.atomicLightCount.ID  ,0, GL_RGBA , GL_UNSIGNED_BYTE , &clear ));
    uint null[4]{0,1,1,0};
    lightBuffer.atomicCounter.FillBuffer(null , 16 , 0);

    GLCall(glViewport(0,0,240,135));
    GLCall(glDisable(GL_STENCIL_TEST));
    GLCall(glDisable(GL_DEPTH_TEST));
    GLCall(glEnable(GL_CULL_FACE));
    GLCall(glCullFace(GL_FRONT));
    lightRasterPreShader.Bind();
    state.lightVao.GetVao().Bind();
    GLCall(glMemoryBarrierByRegion(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT));
    GLCall(glDrawArraysInstanced(GL_TRIANGLES , lightVolumeMesh.indOffset/4 , lightVolumeMesh.indSize/4 , state.lightVao.cmdCount));

    lightCullComputeShader.Bind();
    GLCall(glMemoryBarrierByRegion(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT));
    GLCall(glDispatchCompute(40,1,1));

    lightRasterPostShader.Bind();
    GLCall(glMemoryBarrierByRegion(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT));
    GLCall(glDrawArraysInstanced(GL_TRIANGLES , lightVolumeMesh.indOffset/4 , lightVolumeMesh.indSize/4 , state.lightVao.cmdCount));
    std::cout << state.lightVao.cmdCount << std::endl;

    GLCall(glViewport(0,0,1920,1080));
    tiledLightComputeShader.Bind();
    GLCall(glBindBuffer( GL_DISPATCH_INDIRECT_BUFFER , lightBuffer.atomicCounter.GetID() ));
    GLCall(glBindBufferBase(GL_SHADER_STORAGE_BUFFER , 3, state.matrixLightBuffer.buffer.GetID() ));
    GLCall(glMemoryBarrierByRegion(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_FRAMEBUFFER_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT));
    GLCall(glDispatchComputeIndirect(0));

    //GLCall(glMemoryBarrierByRegion(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT));
    //fogComputeShader.Bind();
    //GLCall(glDispatchComputeIndirect(0));

    // if(s) {
    //     GLCall(glMemoryBarrierByRegion(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT|GL_FRAMEBUFFER_BARRIER_BIT));
    //     GLCall(glEnable(GL_CULL_FACE));
    //     GLCall(glDepthMask(GL_FALSE));
    //     GLCall(glDisable(GL_DEPTH_TEST));
    //     GLCall(glEnable(GL_BLEND));
    //     GLCall(glBlendFunc(GL_ONE,GL_ONE));
    //     fogShadowShader.Bind();
    //     ((VertexArray*)volShadowVao)->Bind();
    //     GLCall(glCullFace(GL_BACK));
    //     GLCall(glBlendEquation(GL_FUNC_ADD));
    //     GLCall(glDrawTransformFeedback(GL_TRIANGLES , tranformFeedbackState));

    //     GLCall(glCullFace(GL_FRONT));
    //     GLCall(glBlendEquation(GL_FUNC_REVERSE_SUBTRACT));
    //     GLCall(glDrawTransformFeedback(GL_TRIANGLES , tranformFeedbackState));
    // }
   
    // GLCall(glBlendEquation(GL_FUNC_ADD));
    GLCall(glMemoryBarrierByRegion(GL_TEXTURE_FETCH_BARRIER_BIT));
}

void RenderLoop(Renderer* r) {

    std::cout << "Render Thread started: " << std::this_thread::get_id() << std::endl;
    r->render.Bind();
    GLFWCall(glfwSwapInterval(0));

    r->global.rc.renderLck.lock();
    r->global.InitServer();
    r->text.InitServer();
    r->raster.InitServer();
    r->gPass.InitServer();
    r->global.rc.renderLck.unlock();

    while(!r->shouldClose) {
        auto timer = std::chrono::high_resolution_clock::now();
        r->global.rc.renderLck.lock();
        r->global.BeginDrawServer();
        r->text.BeginDrawServer();
        r->raster.BeginDrawServer();
        r->gPass.BeginDrawServer();
        r->global.c.stateLck.unlock();

        r->gPass.DrawServer();
        r->raster.DrawServer();
        //r->text.DrawServer();

        r->gPass.EndDrawServer();
        r->raster.EndDrawServer();
        r->text.EndDrawServer();
        r->global.EndDrawServer();

        r->render.SwapBuffers();
        r->global.c.stateLck.unlock();

        r->global.rc.renderLck.unlock();

        auto end = std::chrono::high_resolution_clock::now();
        auto deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(end-timer).count();
        //std::cout << deltaTime << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    r->global.rc.renderLck.lock();
    r->gPass.CleanUpServer();
    r->raster.CleanUpServer();
    r->text.CleanUpServer();
    r->global.CleanUpServer();
    r->global.rc.renderLck.unlock();

    r->render.UnBind();
    std::cout << "Render thread Exit" << std::endl;
}