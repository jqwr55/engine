#include <cstring>
#include <filesystem>

#include <Renderer.h>
#include <meta_info.h> // order

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

constexpr uint requiredExtensionsCount = 54;
constexpr const char* requiredExtensions[54] = {
    "GL_AMD_multi_draw_indirect",
    "GL_ARB_base_instance",
    "GL_ARB_base_instance",
    "GL_ARB_buffer_storage",
    "GL_ARB_clear_texture",
    "GL_ARB_compute_shader",
    "GL_ARB_copy_buffer",
    "GL_ARB_copy_image",
    "GL_ARB_depth_clamp",
    "GL_ARB_direct_state_access",
    "GL_ARB_draw_indirect",
    "GL_ARB_draw_elements_base_vertex",
    "GL_ARB_enhanced_layouts",
    "GL_ARB_explicit_attrib_location",
    "GL_ARB_explicit_uniform_location",
    "GL_ARB_geometry_shader4",
    "GL_ARB_gpu_shader_fp64",
    "GL_ARB_gpu_shader_int64",
    "GL_ARB_half_float_pixel",
    "GL_ARB_half_float_vertex",
    "GL_ARB_indirect_parameters",
    "GL_ARB_instanced_arrays",
    "GL_ARB_map_buffer_alignment",
    "GL_ARB_map_buffer_range",
    "GL_ARB_multi_draw_indirect",
    "GL_ARB_shader_atomic_counter_ops",
    "GL_ARB_shader_atomic_counter_ops",
    "GL_ARB_shader_group_vote",
    "GL_ARB_shader_image_load_store",
    "GL_ARB_shader_bit_encoding",
    "GL_ARB_shader_image_size",
    "GL_ARB_shader_storage_buffer_object",
    "GL_ARB_shader_subroutine",
    "GL_ARB_shader_texture_lod",
    "GL_ARB_shading_language_420pack",
    "GL_ARB_shadow",
    "GL_ARB_stencil_texturing",
    "GL_ARB_texture_compression",
    "GL_ARB_texture_compression_bptc",
    "GL_ARB_texture_compression_rgtc",
    "GL_ARB_texture_storage",
    "GL_ARB_transform_feedback3",
    "GL_ARB_vertex_array_object",
    "GL_ARB_vertex_buffer_object",
    "GL_ARB_vertex_attrib_64bit",
    "GL_S3_s3tc",
    "GL_EXT_blend_subtract",
    "GL_EXT_compiled_vertex_array",
    "GL_EXT_packed_depth_stencil",
    "GL_EXT_stencil_two_side",
    "GL_KHR_debug",
    "GL_KHR_no_error",
    "GL_KHR_shader_subgroup",
    "GL_NVX_gpu_memory_info",
};

void OnCursorPos(GLFWwindow* context,double x , double y) {
    GLFWCall(Renderer* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(context)));
    renderer->settings.OnCursorPos( renderer->settings.client , x,y );
}
void OnClose(GLFWwindow* context) {
    GLFWCall(Renderer* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(context)));
    renderer->settings.NotifyClientWindowClose( renderer->settings.client );
}
void OnResize(GLFWwindow* context , int x , int y) {
    GLFWCall(Renderer* renderer = static_cast<Renderer*>(glfwGetWindowUserPointer(context)));
    renderer->settings.OnResize( renderer->settings.client , x,y );
}

void UpdateVaos(Renderer* renderer , RenderState* state) {

    if( renderer->buffers.vaosDirty ) {
        for(u32 i = 0 ; i < VAO_COUNT ; i++) {
            state->vaos[i].state |= VAO_ATTACH;
        }
        renderer->buffers.dirtyVaoCount--;
        renderer->buffers.vaosDirty = (renderer->buffers.dirtyVaoCount == 0 ? false : true);
    }
    for(u32 i = 0; i < VAO_COUNT ; i++) {
        if( (state->vaos[i].state & VAO_INIT) == VAO_INIT) {
            GLCall(glGenVertexArrays(1,&(state->vaos[i].gl_handle)));
            state->vaos[i].index = 0;
            state->vaos[i].state &= ~VAO_INIT;
        }
    }
    if((state->vaos[VAO_DEBUG].state & VAO_ATTACH) == VAO_ATTACH) {
        state->vaos[VAO_DEBUG].index = 0;
        AttachVBO(state->vaos[VAO_DEBUG] ,renderer->descriptors[VERTEX_COL_DESCRIPTOR] , state->matrixBuffer );
        state->vaos[VAO_DEBUG].state &= ~VAO_ATTACH;
    }
    if((state->vaos[VAO_GEOMETRY].state & VAO_ATTACH) == VAO_ATTACH) {
        state->vaos[VAO_GEOMETRY].index = 0;
        AttachVBO(state->vaos[VAO_GEOMETRY] ,renderer->descriptors[VERTEX_UV_DESCRIPTOR] , renderer->buffers.meshVertexBuffer );
        AttachVBO(state->vaos[VAO_GEOMETRY] ,renderer->descriptors[MATRIX_DESCRIPTOR] , state->matrixBuffer );
        AttachIBO(state->vaos[VAO_GEOMETRY] ,renderer->buffers.meshIndexBuffer);
        state->vaos[VAO_GEOMETRY].state &= ~VAO_ATTACH;
    }
    if((state->vaos[VAO_TEXTURED].state & VAO_ATTACH) == VAO_ATTACH) {
        state->vaos[VAO_TEXTURED].index = 0;
        AttachVBO(state->vaos[VAO_TEXTURED] ,renderer->descriptors[VERTEX_UV_DESCRIPTOR] , renderer->buffers.meshVertexBuffer );
        AttachVBO(state->vaos[VAO_TEXTURED] ,renderer->descriptors[TEXTURE_MATRIX_DESCIRPTOR] , state->matrixBuffer );
        AttachIBO(state->vaos[VAO_TEXTURED] ,renderer->buffers.meshIndexBuffer);
        state->vaos[VAO_TEXTURED].state &= ~VAO_ATTACH;
    }
    
}

void UpdateShaders(Renderer* const renderer) {
    for(u32 i = 0 ; i < SHADER_COUNT ; i++) {   
        auto s = std::filesystem::last_write_time(shaderInfoTable[i].filePath);
        if( s != renderer->shaders[i].time ) {
            u32 new_handle = CreateShaderHandle( shaderInfoTable[i].filePath,shaderInfoTable[i].names,shaderInfoTable[i].nameCount );
            if(new_handle != 0) {
                GLCall(glDeleteProgram(renderer->shaders[i].program_handle));
                renderer->shaders[i].program_handle = new_handle;
            }
            renderer->shaders[i].time = s;
        }
    }
}
void TokenStreamPushViewPortSize(Renderer* renderer, u32 x, u32 y, u32 sizeX , u32 sizeY) {
    renderer->tokenStreamLck.lock();

    for(i32 i = 0; i < sizeof(sizeY) ; i++) {
        renderer->tokenByteStream.PushBack( ((byte*)(&sizeY))[i] );
    }
    for(i32 i = 0; i < sizeof(sizeX) ; i++) {
        renderer->tokenByteStream.PushBack( ((byte*)(&sizeX))[i] );
    }
    for(i32 i = 0; i < sizeof(y) ; i++) {
        renderer->tokenByteStream.PushBack( ((byte*)(&y))[i] );
    }
    for(i32 i = 0; i < sizeof(x) ; i++) {
        renderer->tokenByteStream.PushBack( ((byte*)(&x))[i] );
    }

    TokenHead head = TOKEN_VIEWPORT_SIZE;
    for(i32 i = 0; i < sizeof(head) ; i++) {
        renderer->tokenByteStream.PushBack( ((byte*)(&head))[i] );
    }

    renderer->tokenStreamLck.unlock();
}
void TokenStreamPushVaoDelete(Renderer* renderer , u32 vao) {
    renderer->tokenStreamLck.lock();

    u32 v = vao;
    for(i32 i = 0 ; i < sizeof(v); i++) {
        renderer->tokenByteStream.PushBack( ((byte*)(&v))[i] );
    }

    TokenHead head = TOKEN_VAO_DELETE;
    for(i32 i = 0 ; i < sizeof(head); i++) {
        renderer->tokenByteStream.PushBack( ((byte*)(&head))[i] );
    }

    renderer->tokenStreamLck.unlock();
}
void TokenStreamPushMakeTextureResident(Renderer* renderer , u64 address , u32 index) {
    renderer->tokenStreamLck.lock();

    for(i32 i = 0 ; i < sizeof(index); i++) {
        renderer->tokenByteStream.PushBack( ((byte*)(&index))[i] );
    }
    for(i32 i = 0 ; i < sizeof(address); i++) {
        renderer->tokenByteStream.PushBack( ((byte*)(&address))[i] );
    }
    TokenHead head = TOKEN_TEXTURE_MAKE_RESIDENT;
    for(i32 i = 0 ; i < sizeof(head); i++) {
        renderer->tokenByteStream.PushBack( ((byte*)(&head))[i] );
    }

    renderer->tokenStreamLck.unlock();
}
void TokenStreamPushMakeTextureNonResident(Renderer* renderer , u64 address , u32 index) {
    renderer->tokenStreamLck.lock();

    for(i32 i = 0 ; i < sizeof(index); i++) {
        renderer->tokenByteStream.PushBack( ((byte*)(&index))[i] );
    }
    for(i32 i = 0 ; i < sizeof(address); i++) {
        renderer->tokenByteStream.PushBack( ((byte*)(&address))[i] );
    }
    TokenHead head = TOKEN_TEXTURE_MAKE_NON_RESIDENT;
    for(i32 i = 0 ; i < sizeof(head); i++) {
        renderer->tokenByteStream.PushBack( ((byte*)(&head))[i] );
    }

    renderer->tokenStreamLck.unlock();
}
void ConsumeTokens(Renderer* renderer) {

    renderer->tokenStreamLck.lock();

    bool running = true;
    while(running && renderer->tokenByteStream.count > 0) {

        TokenHead head;
        for(i32 i = sizeof(head)-1 ; i >= 0; i--) {
            ((byte*)(&head))[i] = renderer->tokenByteStream.PopBack();
        }
        switch(head) {
        case TOKEN_VIEWPORT_SIZE:
            {
                u32 x,y,sizeX,sizeY;
                for(i32 i = sizeof(x)-1; i >= 0 ; i--) {
                    (((byte*)&x))[i] = renderer->tokenByteStream.PopBack();
                }
                for(i32 i = sizeof(y)-1; i >= 0 ; i--) {
                    (((byte*)&y))[i] = renderer->tokenByteStream.PopBack();
                }
                for(i32 i = sizeof(sizeX)-1; i >= 0 ; i--) {
                    (((byte*)&sizeX))[i] = renderer->tokenByteStream.PopBack();
                }
                for(i32 i = sizeof(sizeY)-1; i >= 0 ; i--) {
                    (((byte*)&sizeY))[i] = renderer->tokenByteStream.PopBack();
                }
                GLCall(glViewport(x,y,sizeX,sizeY)); 

            }
            break;
        case TOKEN_TEXTURE_MAKE_RESIDENT:
            {
                u64 address;
                for(i32 i = sizeof(address)-1 ; i >= 0; i--) {
                    ((byte*)(&address))[i] = renderer->tokenByteStream.PopBack();
                }
                u32 index;
                for(i32 i = sizeof(index)-1 ; i >= 0; i--) {
                    ((byte*)(&index))[i] = renderer->tokenByteStream.PopBack();
                }
                GLCall(glMakeTextureHandleResidentARB(address));
                ASSERT(glIsTextureHandleResidentARB(address));

                renderer->textureAddresses.FillBufferD(&address , sizeof(u64) , index * sizeof(u64) );
            }
            break;
        case TOKEN_TEXTURE_MAKE_NON_RESIDENT:
            {
                u64 address;
                for(i32 i = sizeof(address)-1 ; i >= 0; i--) {
                    ((byte*)(&address))[i] = renderer->tokenByteStream.PopBack();
                }
                u32 index;
                for(i32 i = sizeof(index)-1 ; i >= 0; i--) {
                    ((byte*)(&index))[i] = renderer->tokenByteStream.PopBack();
                }
                address = renderer->textures[TEXTURE_NULL].GetAddress();
                GLCall(glMakeTextureHandleNonResidentARB(address));
                ASSERT(!glIsTextureHandleResidentARB(address));
                renderer->textureAddresses.FillBufferD(&address , sizeof(u64) , index * sizeof(u64) );
            }

            break;
        case TOKEN_VAO_DELETE:
            {
                u32 v;
                for(i32 i = sizeof(v)-1 ; i >= 0; i--) {
                    ((byte*)(&v))[i] = renderer->tokenByteStream.PopBack();
                }
                GLCall(glDeleteVertexArrays(1,&v));
            }
            break;            
        
        default:
            running = false;
            ASSERT(false);
            break;
        }

    }

    renderer->tokenByteStream.Clear();
    renderer->tokenStreamLck.unlock();
}

void ThreadInit(Renderer* renderer) {
    GLFWCall(glfwMakeContextCurrent(renderer->renderContext));
    GLFWCall(glfwSwapInterval(0));
    renderer->paramUbo.BindBase(1);
    renderer->textureAddresses.BindBase(2);
}
void ThreadDeInit(Renderer* renderer) {
    GLFWCall(glfwMakeContextCurrent(0));
}

void RenderLoop(Renderer* renderer) {

    std::cout << "Render Thread started: " << std::this_thread::get_id() << std::endl;
    ThreadInit(renderer);

    while(renderer->shouldRender) {

        RenderState* currentState = nullptr;
        renderer->renderLck.lock();
        renderer->stateLck.lock();
        if( renderer->stateUpdate ) {
            renderer->state = renderer->newState;
            renderer->newState = nullptr;
            renderer->stateUpdate = false;
        }
        renderer->stateLck.unlock();
        ASSERT(renderer->state != nullptr);
        currentState = renderer->state;

        ConsumeTokens(renderer);
        UpdateVaos(renderer,renderer->state);
        UpdateShaders(renderer);

        auto timer = std::chrono::high_resolution_clock::now();
        if(renderer->state->shaderParamDirtyFlag) {
            *((CommonShaderParameteres*)renderer->buffers.pinned.ptr) = renderer->state->shaderParams;
            renderer->state->shaderParamDirtyFlag = false;
            GLCall(glCopyNamedBufferSubData(renderer->buffers.pinned.buffer.gl_handle,renderer->paramUbo.gl_handle,0,0, sizeof(CommonShaderParameteres) ));
        }

        GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
        GLCall(glEnable(GL_DEPTH_TEST));
        GLCall(glEnable(GL_CULL_FACE));
        GLCall(glBindBuffer(GL_DRAW_INDIRECT_BUFFER,currentState->cmdBuffer.gl_handle));

        GLCall(glUseProgram(renderer->shaders[SHADER_GEOMETRY].program_handle));
        GLCall(glBindVertexArray(renderer->state->vaos[VAO_TEXTURED].gl_handle));
        GLCall(glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT ,nullptr , currentState->cmdCount , 0));

        GLCall(glUseProgram(renderer->shaders[SHADER_DEBUG].program_handle));
        GLCall(glBindVertexArray(renderer->state->vaos[VAO_DEBUG].gl_handle));
        GLCall(glLineWidth(3.f));
        GLCall(glDrawArrays(GL_LINES , currentState->lineOffset , currentState->lineCount << 1));
        

        GLCall(glFinish());
        auto end = std::chrono::high_resolution_clock::now();
        auto deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(end-timer).count();
        std::cout << deltaTime << std::endl;

        GLFWCall(glfwSwapBuffers(renderer->renderContext));
        renderer->renderLck.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    ThreadDeInit(renderer);
    std::cout << "Render thread Exit" << std::endl;
}

extern "C" const u32 SIZEOF_RENDERER() {
    return sizeof(Renderer);
}
extern "C" const u32 SIZEOF_RENDERER_SETINGS_STATE() {
    return sizeof(RendererSettingsState);
}
extern "C" const u32 SIZEOF_RENDER_STATE() {
    return sizeof(RenderState);
}

template<typename T> struct CoutPrint {
    static void Func(i32 indentLevel , void* user , T val) {
        if constexpr( std::is_same<T,const char*>::value ) {
            std::cout << std::endl << std::setw(indentLevel*7) << val << " ";
        }
        else {
            std::cout << std::setw(indentLevel*7) << val;
        }
    }
};

template< template<typename> typename T, u32 c>  void DumpStruct( void* basePtr , StructMemberInfoType (&info)[c] ,void* user , i32 indentLevel = 0) {
    constexpr auto member_count = (sizeof(info) / sizeof(StructMemberInfoType));

    for(u32 i = 0; i < member_count ; i++) {
        
        T<const char*>::Func(indentLevel,user, info[i].name );
        void* memberPtr = ((byte*)basePtr + info[i].offset);

        switch (info[i].type) {
            META_TYPE_CASE_DUMP(memberPtr,indentLevel,T,user)
        }
    }

    if( indentLevel == 0 ) {
        T<const char*>::Func(0,user,"");
    }
}


bool CheckExtensionSupport(u32* toalAvailableMemoryEnum , u32* dedicatedVramEnum , u32* currentVramEnum) {

    int numberOfExtensions;
    GLCall(glGetIntegerv(GL_NUM_EXTENSIONS, &numberOfExtensions));

    bool missing = true;
    for(int i = 0; i < requiredExtensionsCount ; i++ ) {

        
        bool found = false;
        for(int k = 0; k < numberOfExtensions ; k++) {
            const char* EXT = (char*)glGetStringi(GL_EXTENSIONS, k);
            if( strcmp( EXT , requiredExtensions[i] ) == 0 ) {
                found = true;
                break;
            }
        }
        if(!found) {
            missing = false;
            std::cout << requiredExtensions[i] << " not supported" << std::endl;
        }
    }

    for(u32 k = 0; k < numberOfExtensions ;k++) {
        const char* EXT = (char*)glGetStringi(GL_EXTENSIONS,k);
        if( strcmp(EXT , "GL_NVX_gpu_memory_info") == 0 ) {
            *toalAvailableMemoryEnum = GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX;
            *dedicatedVramEnum = GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX;
            *currentVramEnum = GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX;
            break;
        }
    }

    return missing;
}

extern "C" Renderer* ConstructRenderer(void* mem , RendererArgs* args) {
    Renderer* r = static_cast<Renderer*>(mem);
    memset(r , 0, sizeof(Renderer));

    r->renderStateRefCount = 0;
    r->settings.Allocate = args->Allocate;
    r->settings.AllocateAligned = args->AllocateAligned;
    r->settings.Free = args->Free;
    r->settings.client = args->client;
    r->settings.windowName = args->windowName;
    r->settings.windowHeight = args->windowHeight;
    r->settings.windowWidth = args->windowWidth;
    r->settings.NotifyClientWindowClose = args->NotifyClientWindowClose;
    r->settings.OnCursorPos = args->OnCursorPos;
    r->settings.callbackCount = args->callbackCount;
    r->settings.callbacks = args->callbacks;
    r->settings.OnResize = args->OnResize;
    r->stateUpdate = false;
    r->newState = nullptr;
    r->state = nullptr;

    new(&(r->renderLck)) std::mutex;
    new(&(r->stateLck)) std::mutex;
    new(&(r->tokenStreamLck)) std::mutex;
    new(&(r->textureHandleLck)) std::mutex;

    VectorBase::Allocate = args->Allocate;
    VectorBase::AllocateAligned = args->AllocateAligned;
    VectorBase::Free = args->Free;
    VectorBase::client = args->client;

    r->tokenByteStream.Clear();
    r->stackBase = args->AllocateAligned( args->client, 4096 , 4096 );
    r->stackPtr = r->stackBase;
    r->textureHandleTable = (u64*)args->Allocate( args->client , sizeof(u64) * 500 );
    memset(r->textureHandleTable, 0 , sizeof(u64) * 500);


    GLFWCall(glfwInit());
    GLFWCall(glfwMakeContextCurrent(0));
    
    const u32 versionMajor = 4;
    const u32 versionMinor = 6;

    GLFWCall(glfwWindowHint(GLFW_CONTEXT_NO_ERROR, GLFW_FALSE ));
    GLFWCall(glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE ));
    GLFWCall(glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE));
    GLFWCall(glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, versionMajor));
    GLFWCall(glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, versionMinor));
    GLFWCall(glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE));
    GLFWCall(r->mainContext = glfwCreateWindow( 1,1, "" , NULL , NULL ));
    ASSERT(r->mainContext);

    GLFWCall(glfwWindowHint(GLFW_CONTEXT_NO_ERROR, GLFW_FALSE ));
    GLFWCall(glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE ));
    GLFWCall(glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE));
    GLFWCall(glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, versionMajor));
    GLFWCall(glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, versionMinor));

    GLFWCall(glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE));
    GLFWCall(r->renderContext = glfwCreateWindow( args->windowWidth , args->windowHeight , args->windowName , NULL , r->mainContext ));
    ASSERT(r->renderContext);
    
    GLFWCall(glfwSetWindowUserPointer(r->renderContext , r));
    GLFWCall(glfwSetWindowCloseCallback(r->renderContext ,OnClose ));
    GLFWCall(glfwSetCursorPosCallback(r->renderContext ,OnCursorPos ));
    GLFWCall(glfwSetWindowSizeCallback(r->renderContext, OnResize));
    
    GLFWCall(glfwMakeContextCurrent(r->mainContext));

    u32 init = glewInit();
    if(init != GLEW_OK) {
        std::cout << "GLEW Error: " << glewGetErrorString(init) << std::endl;
        GLFWCall(glfwDestroyWindow(r->renderContext));
        GLFWCall(glfwDestroyWindow(r->mainContext));
        glfwTerminate();
        return nullptr;
    }

    u32 toalAvailableMemoryEnum,dedicatedVramEnum,currentVramEnum;
    if(!CheckExtensionSupport(&toalAvailableMemoryEnum,&dedicatedVramEnum,&currentVramEnum)) {
        GLFWCall(glfwDestroyWindow(r->renderContext));
        GLFWCall(glfwDestroyWindow(r->mainContext));
        glfwTerminate();
        return nullptr;
    }
    GLCall(std::cout << glGetString(GL_VERSION) << " Context initialized " << std::endl);

    GLCall(glGetIntegerv(toalAvailableMemoryEnum , &(r->toalAvailableMemory)));
    GLCall(glGetIntegerv(dedicatedVramEnum , &(r->dedicatedVram)));
    r->currentVramENUM = currentVramEnum;

    UpdateShaders(r);
    r->descriptors[VERTEX_COL_DESCRIPTOR].Begin(r->stackPtr);
    r->descriptors[VERTEX_COL_DESCRIPTOR].PushAttribute(3 , GL_FLOAT , SHADER_TYPE_FLOAT , GL_FALSE  , 0);
    r->descriptors[VERTEX_COL_DESCRIPTOR].PushAttribute(3 , GL_FLOAT , SHADER_TYPE_FLOAT , GL_FALSE  , 0);
    r->descriptors[VERTEX_COL_DESCRIPTOR].End(r->stackPtr);

    r->descriptors[VERTEX_UV_DESCRIPTOR].Begin(r->stackPtr);
    r->descriptors[VERTEX_UV_DESCRIPTOR].PushAttribute(3 , GL_FLOAT , SHADER_TYPE_FLOAT , GL_FALSE , 0);
    r->descriptors[VERTEX_UV_DESCRIPTOR].PushAttribute(2 , GL_FLOAT , SHADER_TYPE_FLOAT , GL_FALSE , 0);
    r->descriptors[VERTEX_UV_DESCRIPTOR].End(r->stackPtr);

    r->descriptors[MATRIX_DESCRIPTOR].Begin(r->stackPtr);
    r->descriptors[MATRIX_DESCRIPTOR].PushAttribute(3 , GL_FLOAT , SHADER_TYPE_FLOAT , GL_FALSE , 1);
    r->descriptors[MATRIX_DESCRIPTOR].PushAttribute(3 , GL_FLOAT , SHADER_TYPE_FLOAT , GL_FALSE , 1);
    r->descriptors[MATRIX_DESCRIPTOR].PushAttribute(3 , GL_FLOAT , SHADER_TYPE_FLOAT , GL_FALSE , 1);
    r->descriptors[MATRIX_DESCRIPTOR].PushAttribute(3 , GL_FLOAT , SHADER_TYPE_FLOAT , GL_FALSE , 1);
    r->descriptors[MATRIX_DESCRIPTOR].End(r->stackPtr);

    r->descriptors[TEXTURE_MATRIX_DESCIRPTOR].Begin(r->stackPtr);
    r->descriptors[TEXTURE_MATRIX_DESCIRPTOR].PushAttribute(3 , GL_FLOAT , SHADER_TYPE_FLOAT , GL_FALSE , 1);
    r->descriptors[TEXTURE_MATRIX_DESCIRPTOR].PushAttribute(3 , GL_FLOAT , SHADER_TYPE_FLOAT , GL_FALSE , 1);
    r->descriptors[TEXTURE_MATRIX_DESCIRPTOR].PushAttribute(3 , GL_FLOAT , SHADER_TYPE_FLOAT , GL_FALSE , 1);
    r->descriptors[TEXTURE_MATRIX_DESCIRPTOR].PushAttribute(3 , GL_FLOAT , SHADER_TYPE_FLOAT , GL_FALSE , 1);
    r->descriptors[TEXTURE_MATRIX_DESCIRPTOR].PushAttribute(1 , GL_UNSIGNED_INT , SHADER_TYPE_INT , GL_FALSE , 1);
    r->descriptors[TEXTURE_MATRIX_DESCIRPTOR].End(r->stackPtr);

    new(&(r->textureAddresses)) BufferObject<GL_UNIFORM_BUFFER>;
    new(&(r->paramUbo)) BufferObject<GL_UNIFORM_BUFFER>;
    new(&(r->buffers)) CommonBuffers();

    {
        for(u32 i = 0; i < TEXTURE_COUNT ; i++) {
            r->textures[i].Init();
            r->textures[i].Bind<GL_TEXTURE_2D>();
        }
        int x,y,n;
        auto pixelData = stbi_load("../res/textures/missing_texture.jpg",&x,&y,&n,4);
        r->textures[TEXTURE_NULL].ImmutableStorage<GL_TEXTURE_2D>( GL_RGBA8 , 1, x,y,1 );
        r->textures[TEXTURE_NULL].Upload<GL_TEXTURE_2D>(pixelData, GL_RGBA , GL_UNSIGNED_BYTE , 0, x,y,1);

        u64 address = r->textures[TEXTURE_NULL].GetAddress();
        for(u32 i = 0; i < 500 ; i++) {
            r->textureHandleTable[i] = address;
        }
        TokenStreamPushMakeTextureResident(r , address , 0);
    }

    
    r->paramUbo.ImmutableStorage(nullptr , sizeof(CommonShaderParameteres) , 0);
    r->textureAddresses.ImmutableStorage(r->textureHandleTable , sizeof(u64) * 500 , GL_DYNAMIC_STORAGE_BIT);

    r->stoped = false;
    r->shouldRender = true;
    new( &(r->renderThread) ) std::thread(RenderLoop,r);
    StopRenderer(r);

    return r;
}

extern "C" void DestroyRenderer(Renderer* renderer) {

    renderer->shouldRender = false;
    renderer->renderThread.join();
    renderer->renderThread.~thread();

    renderer->buffers.~CommonBuffers();
    renderer->paramUbo.~BufferObject();
    renderer->textureAddresses.~BufferObject();

    for(i32 i = DESCRIPTOR_COUNT-1; i >= 0 ; --i) {
        renderer->descriptors[i].Free(renderer->stackPtr);
    }
    for(u32 i = 0; i < SHADER_COUNT ; i++) {
        GLCall(glDeleteProgram(renderer->shaders[i].program_handle));
    }
    for(u32 i = 0; i < TEXTURE_COUNT ; i++) {
        renderer->textures[i].~BindlessTexture();
    }
    
    GLFWCall(glfwDestroyWindow(renderer->renderContext));
    GLFWCall(glfwDestroyWindow(renderer->mainContext));

    renderer->textureHandleLck.~mutex();
    renderer->tokenStreamLck.~mutex();
    renderer->renderLck.~mutex();
    renderer->stateLck.~mutex();

    renderer->renderStateRefCount = 0;
    renderer->settings.Free( renderer->settings.client , renderer->stackBase );
    renderer->settings.Free( renderer->settings.client , renderer->textureHandleTable );

    memset(renderer, 0 , sizeof(Renderer));
}
extern "C" void SetViewPort(Renderer* renderer , u32 x , u32 y , u32 width , u32 height) {
    TokenStreamPushViewPortSize(renderer , x,y, width , height);
    renderer->settings.windowWidth = width;
    renderer->settings.windowHeight = height;
}

void LoadSettingsState(Renderer* renderer) {
    GLFWCall(glfwSetWindowSize(renderer->renderContext , renderer->settings.windowWidth , renderer->settings.windowHeight));
    GLFWCall(glfwSetWindowTitle(renderer->renderContext , renderer->settings.windowName));
    TokenStreamPushViewPortSize(renderer , 0,0, renderer->settings.windowWidth , renderer->settings.windowHeight);
}
extern "C" void SetRendererSettingsState(Renderer* renderer , RendererSettingsState* state) {
    renderer->settings = *state;
    LoadSettingsState(renderer);
}
extern "C" RendererSettingsState* GetRendererSettingsState(Renderer* renderer , void* mem) {
    RendererSettingsState* ret = static_cast<RendererSettingsState*>(mem);
    *ret = renderer->settings;
    return ret;
}
extern "C" void DumpRendererSettingsCout(RendererSettingsState* settings) {
    DumpStruct<CoutPrint>(settings , meta_info_of_RendererSettingsState , nullptr);
}
extern "C" void PollEvents(Renderer* r) {

    GLFWCall(glfwPollEvents());
    for(u32 i = 0; i < r->settings.callbackCount;i++) {
        if( glfwGetKey(r->renderContext ,r->settings.callbacks[i].key ) == GLFW_PRESS ) {
            r->settings.callbacks[i].func( r->settings.callbacks[i].user );
        }
    }
}

extern "C" void SetCursorPos(Renderer* renderer, double x, double y) {
    GLFWCall(glfwSetCursorPos(renderer->renderContext , x,y));
}
extern "C" void GetCursorPos(Renderer* renderer , double* x, double* y) {
    GLFWCall(glfwGetCursorPos(renderer->renderContext, x,y));
}

extern "C" bool ReadyToRender(Renderer* renderer) {
    return !renderer->stateUpdate;
}

extern "C" RenderState* ConstructRenderState(void* mem, Renderer* renderer) {
    RenderState* ret = static_cast<RenderState*>(mem);
    memset(ret, 0, sizeof(RenderState));  

    renderer->renderStateRefCount++;

    for(u32 i = 0; i < VAO_COUNT ; i++) {
        ret->vaos[i].state = VAO_INIT | VAO_ATTACH;
    }
    ret->settings = &renderer->settings;
    ret->buffers = &renderer->buffers;

    const GLbitfield storage = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_CLIENT_STORAGE_BIT,map = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT;

    new(&(ret->pinned)) PersistentlyMappedBuffer<GL_COPY_READ_BUFFER>(4 * MEGA_BYTES , storage , map);
    new(&(ret->matrixBuffer)) BufferObject<GL_ARRAY_BUFFER>;
    new(&(ret->cmdBuffer)) BufferObject<GL_DRAW_INDIRECT_BUFFER>;

    ret->matrixBuffer.ImmutableStorage(nullptr ,4 * KILO_BYTES , 0);
    ret->cmdBuffer.ImmutableStorage(nullptr ,4 * KILO_BYTES , 0);

    return ret;
}

extern "C" void DestroyRenderState(Renderer* renderer , RenderState* state ) {
    
    renderer->renderStateRefCount--;
    state->cmdBuffer.~BufferObject();
    state->matrixBuffer.~BufferObject();
    state->pinned.buffer.~BufferObject();

    memset(state , 0, sizeof(RenderState));
}

extern "C" void BeginRenderState(Renderer* renderer, RenderState* state) {
    state->byteOffset = 0;
    state->cmdCount = 0;
    state->lineCount = 0;
    state->lineOffset = 0;
}
extern "C" void EndRenderState(Renderer* renderer , RenderState* state) {
    GLCall(glFinish());
}
extern "C" CommonShaderParameteres* GetShaderParams(RenderState* state) {
    return &state->shaderParams;
}
extern "C" void ReleaseShaderParams(RenderState* state) {
    state->shaderParamDirtyFlag = true;
}
extern "C" void ProcessState(RenderState* state) {
    
    if(state->matrixBuffer.m_size < state->byteOffset + 64 ) {
        state->matrixBuffer.ImmutableResize(nullptr , state->byteOffset + 64 , 0);
        state->vaos[VAO_DEBUG].state |= VAO_ATTACH;
        state->vaos[VAO_GEOMETRY].state |= VAO_ATTACH;
        state->vaos[VAO_TEXTURED].state |= VAO_ATTACH;
    }
    if( state->cmdBuffer.m_size < (state->cmdCount + 1) * sizeof(DrawElementsIndirectCommand) ) {
        state->cmdBuffer.ImmutableResize(nullptr , (state->cmdCount + 1) * sizeof(DrawElementsIndirectCommand) , 0);
    }
    GLCall(glCopyNamedBufferSubData(state->pinned.buffer.gl_handle , state->matrixBuffer.gl_handle , 0 ,0 , state->byteOffset));
    GLCall(glCopyNamedBufferSubData(state->pinned.buffer.gl_handle , state->cmdBuffer.gl_handle, 2 * MEGA_BYTES , 0 , state->cmdCount * sizeof(DrawElementsIndirectCommand) ));

}
extern "C" void SubmitState(Renderer* renderer , RenderState* state) {
    renderer->stateLck.lock();
    renderer->stateUpdate = true;
    renderer->newState = state;
    renderer->stateLck.unlock();
}

extern "C" void StartRenderer(Renderer* renderer) {
    if(renderer->stoped) {
        renderer->stoped = false;
        renderer->renderLck.unlock();
    }
}
extern "C" void StopRenderer(Renderer* renderer) {
    if(!renderer->stoped) {
        renderer->stoped = true;
        renderer->renderLck.lock();
    }
}

extern "C" void InitTexture(Texture* texture) {
    GLCall(glGenTextures(1, &(texture->glID)));
}
extern "C" void FreeTexture(Texture* texture) {
    GLCall(glDeleteTextures(1, &(texture->glID)))
}
extern "C" void TextureStorage(Texture* texture , u32 type , u32 internalFormat , u32 mipLevels, u32 x,u32 y,u32 z) {
    ((BindlessTexture*)texture)->Bind<GL_TEXTURE_2D>();
    ((BindlessTexture*)texture)->ImmutableStorage<GL_TEXTURE_2D>(internalFormat , mipLevels , x,y,z);
}
extern "C" void TextureUpload(Texture* texture ,u32 type, void* data , u32 dataFormat ,u32 dataType ,u32 mipLevel,u32 x,u32 y,u32 z) {
    ((BindlessTexture*)texture)->Bind<GL_TEXTURE_2D>();
    ((BindlessTexture*)texture)->Upload<GL_TEXTURE_2D>(data , dataFormat , dataType , mipLevel ,x,y,z);
}
extern "C" void GenerateMipMapChain(Texture* texture) {
    ((BindlessTexture*)texture)->Bind<GL_TEXTURE_2D>();
    GLCall(glGenerateMipmap(GL_TEXTURE_2D));
}
extern "C" void SetTextureParamaters(Texture* texture , u32 pname[] , i32 params[] , u32 size) {
    ((BindlessTexture*)texture)->Bind<GL_TEXTURE_2D>();
    for(int i = 0; i < size ; i++) {
        GLCall(glTextureParameteri( texture->glID , pname[i] , params[i]));
    }
}
extern "C" u32 MakeTextureResident (Renderer* renderer , Texture* texture) {

    u64 defaultTextureAddress = renderer->textures[TEXTURE_NULL].GetAddress();
    u64 address = ((BindlessTexture*)texture)->GetAddress();
    renderer->textureHandleLck.lock();

    for(u32 i = 1 ; i < 500 ; i++) {
        if( renderer->textureHandleTable[i] == defaultTextureAddress ) {
            TokenStreamPushMakeTextureResident(renderer , address , i);
            renderer->textureHandleTable[i] = address;
            renderer->textureHandleLck.unlock();
            return i;
        }
    }

    renderer->textureHandleLck.unlock();

    return 0;
}
extern "C" void MakeTextureNonResident(Renderer* renderer , u32 index) {
    
    ASSERT(index > 0 && index < 500);
    renderer->textureHandleLck.lock();
    TokenStreamPushMakeTextureNonResident(renderer ,renderer->textureHandleTable[index], index);
    renderer->textureHandleTable[index] = 0;
    renderer->textureHandleLck.unlock();

}

u32 Transfer(PersistentlyMappedBuffer<GL_COPY_READ_BUFFER>& pinned,u32 pinnedStartOffset, void* src, u32 srcSize, u32 dstBuffer, u32 dstOffset, u32 dstSize ) {

    byte* pinnedPtr = (byte*)pinned.ptr + pinnedStartOffset;
    byte* srcPtr = (byte*)src;

    u32 sizeRemaining = srcSize;
    u32 readOffset = pinnedStartOffset;
    u32 writeOffset = dstOffset;

    while(sizeRemaining > 0) {

        ASSERT(writeOffset <= dstSize);
        u32 sizeWritten = min<u32>(pinned.buffer.m_size-pinnedStartOffset , sizeRemaining);
        MemCpy(srcPtr, pinnedPtr , sizeWritten );
        GLCall(glCopyNamedBufferSubData(pinned.buffer.gl_handle , dstBuffer , readOffset , writeOffset , sizeWritten));
        srcPtr += sizeWritten;
        pinnedPtr += sizeWritten;
        readOffset += sizeWritten;
        writeOffset += sizeWritten;
        sizeRemaining -= sizeWritten;

        if( readOffset >= pinned.buffer.m_size ) {
            pinnedPtr = (byte*)pinned.ptr;
            readOffset = 0;
            GLCall(glFinish());
        }
    }

    return readOffset;
}

extern "C" Mesh RegisterMesh(Renderer* renderer, void* verts , u32 vertSize , u32* indices , u32 indSize , bool shadowFlag ) {
    renderer->buffers.lck.lock();

    u32 offsetV = renderer->buffers.meshVertexBuffer.m_size;
    u32 offsetI = renderer->buffers.meshIndexBuffer.m_size;

    u32 new_mesh_handles[2];
    GLCall(glGenBuffers(2,new_mesh_handles));
    GLCall(glBindBuffer(GL_ARRAY_BUFFER,new_mesh_handles[0]));
    GLCall(glNamedBufferStorage(new_mesh_handles[0], renderer->buffers.meshVertexBuffer.m_size + vertSize, nullptr,0));
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,new_mesh_handles[1]));
    GLCall(glNamedBufferStorage(new_mesh_handles[1], renderer->buffers.meshIndexBuffer.m_size + indSize, nullptr,0));

    GLCall(glCopyNamedBufferSubData(renderer->buffers.meshVertexBuffer.gl_handle , new_mesh_handles[0], 0,0,offsetV ));
    GLCall(glCopyNamedBufferSubData(renderer->buffers.meshIndexBuffer.gl_handle , new_mesh_handles[1], 0,0,offsetI ));

    u32 offset = Transfer(renderer->buffers.pinned,0 , verts , vertSize , new_mesh_handles[0] , offsetV, renderer->buffers.meshVertexBuffer.m_size + vertSize);
    offset = Transfer(renderer->buffers.pinned, offset , indices , indSize , new_mesh_handles[1] , offsetI, renderer->buffers.meshIndexBuffer.m_size + indSize);
    
    renderer->buffers.meshVertexBuffer.~BufferObject();
    renderer->buffers.meshIndexBuffer.~BufferObject();

    renderer->buffers.meshVertexBuffer.gl_handle = new_mesh_handles[0];
    renderer->buffers.meshIndexBuffer.gl_handle = new_mesh_handles[1];
     
    renderer->buffers.meshVertexBuffer.m_size += vertSize;
    renderer->buffers.meshIndexBuffer.m_size += indSize;

    renderer->buffers.vaosDirty = true;
    renderer->buffers.dirtyVaoCount = renderer->renderStateRefCount;
    
    MeshInternal mesh{};
    mesh.indexCount = indSize;
    mesh.indexOffset = offsetI;
    mesh.vertexCount = vertSize;
    mesh.vertexOffset = offsetV;

    renderer->buffers.meshHandleTable[++renderer->buffers.meshID] = mesh;

    GLCall(glFinish());
    renderer->buffers.lck.unlock();

    return {renderer->buffers.meshID};;
}
extern "C" void UnRegisterMesh(Renderer* renderer, Mesh mesh) {

    renderer->buffers.lck.lock();
    MeshInternal m = renderer->buffers.meshHandleTable.at(mesh.id);

    u32 new_mesh_handles[2];
    GLCall(glGenBuffers(2,new_mesh_handles));

    GLCall(glBindBuffer(GL_ARRAY_BUFFER,new_mesh_handles[0]));
    GLCall(glNamedBufferStorage(new_mesh_handles[0], renderer->buffers.meshVertexBuffer.m_size - m.vertexCount, nullptr,0));
    
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,new_mesh_handles[1]));
    GLCall(glNamedBufferStorage(new_mesh_handles[1], renderer->buffers.meshIndexBuffer.m_size - m.indexCount, nullptr,0));
    
    GLCall(glCopyNamedBufferSubData(renderer->buffers.meshVertexBuffer.gl_handle, new_mesh_handles[0] , 0,0, m.vertexOffset));
    GLCall(glCopyNamedBufferSubData(renderer->buffers.meshIndexBuffer.gl_handle, new_mesh_handles[1] , 0,0, m.indexOffset));

    if((m.vertexOffset + m.vertexCount) < renderer->buffers.meshVertexBuffer.m_size ) {
        u32 remaining = renderer->buffers.meshVertexBuffer.m_size - (m.vertexOffset + m.vertexCount);
        GLCall(glCopyNamedBufferSubData(renderer->buffers.meshVertexBuffer.gl_handle, new_mesh_handles[0], m.vertexOffset + m.vertexCount ,m.vertexOffset, remaining ));
        remaining = renderer->buffers.meshIndexBuffer.m_size - (m.indexCount + m.indexOffset);
        GLCall(glCopyNamedBufferSubData(renderer->buffers.meshVertexBuffer.gl_handle, new_mesh_handles[1], m.indexOffset + m.indexCount ,m.indexOffset, remaining ));
    }

    renderer->buffers.meshVertexBuffer.~BufferObject();
    renderer->buffers.meshIndexBuffer.~BufferObject();

    renderer->buffers.meshVertexBuffer.gl_handle = new_mesh_handles[0];
    renderer->buffers.meshIndexBuffer.gl_handle = new_mesh_handles[1];

    renderer->buffers.meshVertexBuffer.m_size -= m.vertexCount;
    renderer->buffers.meshIndexBuffer.m_size -= m.indexCount;
    renderer->buffers.vaosDirty = true;
    renderer->buffers.dirtyVaoCount = renderer->renderStateRefCount;

    renderer->buffers.meshHandleTable.erase(mesh.id);

    GLCall(glFinish());
    renderer->buffers.lck.unlock();
}

static _FORCE_INLINE glm::mat4x3 MakeTransform(Transform& t) {

    glm::mat3 m = glm::eulerAngleXYZ(t.r.x , t.r.y , t.r.z);
    m[0][0] *= t.s.x;
    m[1][1] *= t.s.y;
    m[2][2] *= t.s.z;

    glm::mat4x3 r;
    r[0] = m[0];
    r[1] = m[1];
    r[2] = m[2];
    r[3] = t.p;

    return r;
}

static _FORCE_INLINE u32 ALIGN(u32 ptr , u32 size) {
    ptr += (size - (ptr % size)) * (ptr % size != 0);
    return ptr;
}

extern "C" void DrawMeshes(RenderState* state, Mesh* handles,u32* textureIndex , Transform* transforms ,u32 count) {

    u32 uniqueMeshCount[state->buffers->meshID+1]{0};
    MeshInternal m[state->buffers->meshID+1];
    DrawElementsIndirectCommand* cmdPtr = (DrawElementsIndirectCommand*)((byte*)state->pinned.ptr + 2 * MEGA_BYTES);

    state->byteOffset = ALIGN(state->byteOffset , sizeof(MatrixTextureInstanceData) );
    MatrixTextureInstanceData* ptr = (MatrixTextureInstanceData*)((byte*)state->pinned.ptr + state->byteOffset);
    u32 baseInstance = state->byteOffset / sizeof(MatrixTextureInstanceData);
    u32 matrixIndex = 0;

    for(u32 i = 0; i < count ;i++) {
        uniqueMeshCount[handles[i].id]++;
    }
    for(u32 i = 0; i < state->buffers->meshID+1 ; i++) {

        if(uniqueMeshCount[i] == 0) {
            continue;
        }

        for(u32 k = 0; k < count ; k++) {
            if( handles[k].id == i ) {
                ptr[matrixIndex++] = {MakeTransform(transforms[k]) , textureIndex[k]};
            }
        }
    }

    state->buffers->lck.lock();
    for(u32 i = 0; i < state->buffers->meshID+1 ; i++) {
        if(uniqueMeshCount[i] == 0) {
            continue;
        }
        m[i] = state->buffers->meshHandleTable.at(i);

        cmdPtr[state->cmdCount++] = {m[i].indexCount>>2, uniqueMeshCount[i] , m[i].indexOffset>>2 , m[i].vertexOffset / sizeof(Vertex), baseInstance };
        baseInstance += uniqueMeshCount[i];
    }
    state->buffers->lck.unlock();

    state->byteOffset += matrixIndex * sizeof(MatrixTextureInstanceData);
}

extern "C" void DrawLines(RenderState* state, Line* lines , u32 count) {

    state->byteOffset = ALIGN(state->byteOffset , sizeof(ColoredVertex) );
    byte* ptr = (byte*)state->pinned.ptr + state->byteOffset;
    MemCpy(lines , ptr , count * sizeof(Line));

    state->lineOffset = state->byteOffset / sizeof(ColoredVertex);
    state->lineCount = count;

    state->byteOffset += count * sizeof(Line);
}