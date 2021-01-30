#include <InternalFunctions.h>
#include <UtilityFuncs.h>
#include <stdio.h>
#include <string.h>

constexpr uint requiredExtensionsCount = 54;
constexpr const char* requiredExtensions[] = {
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

extern "C" const uint32_t SIZEOF_CONTEXT() {
    return sizeof(Context);
}
extern "C" const uint32_t SIZEOF_RENDERER() {
    return sizeof(Renderer);
}
extern "C" bool ShouldClose(void* r) {
    Renderer* renderer = static_cast<Renderer*>(r);
    return renderer->shouldClose;
}

extern "C" bool IsKeyPressed(void* r , int key) {
    Renderer* renderer = static_cast<Renderer*>(r);
    GLFWCall(return glfwGetKey(renderer->render.GetContext() , key));
}

extern "C" void StartRenderer(void* r) {
    Renderer* renderer = static_cast<Renderer*>(r);
    if(renderer->global.m.locked) {
        renderer->global.rc.renderLck.unlock();
        renderer->global.m.locked = false;
    }
}
extern "C" void StopRenderer(void* r) {
    Renderer* renderer = static_cast<Renderer*>(r);
    
    if(!renderer->global.m.locked) {
        renderer->global.rc.renderLck.lock();
        renderer->global.m.locked = true;
    }
}
extern "C" void CursorVisibility(void* r, bool a) {
    Renderer* renderer = static_cast<Renderer*>(r);
    GLFWCall(glfwSetInputMode(renderer->render.GetContext(), GLFW_CURSOR, (a ? : GLFW_CURSOR_HIDDEN , GLFW_CURSOR_NORMAL) ));
}
extern "C" void Show(void* r) {
    Renderer* renderer = static_cast<Renderer*>(r);
    GLFWCall(glfwShowWindow(renderer->render.GetContext()));
}
extern "C" void Hide(void* r) {
    Renderer* renderer = static_cast<Renderer*>(r);
    GLFWCall(glfwHideWindow(renderer->render.GetContext()));
}
extern "C" void Resize(void* r, uint x , uint y) {
    Renderer* renderer = static_cast<Renderer*>(r);
    renderer->render.Resize(x ,y);
    renderer->render.UpdateDims();
    renderer->global.rc.shouldResize = true;
}
extern "C" void Close(void* r) {
    Renderer* renderer = static_cast<Renderer*>(r);
    renderer->shouldClose = true;
}
extern "C" void Open(void* r) {
    Renderer* renderer = static_cast<Renderer*>(r);
    StopRenderer(r);
    renderer->shouldClose = false;
    renderer->renderThread.__make_invoker(RenderLoop , renderer);
}
extern "C" void SetCursorPos(void* r , uint x, uint y) {
    Renderer* renderer = static_cast<Renderer*>(r);
    GLFWCall(glfwSetCursorPos(renderer->render.GetContext(),x,y));
}

bool CheckExtensionSupport() {
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
    
    int totalAvailableMemory,dedicatedGpuMemory,currentGpuMemory;
    GLCall(glGetIntegerv(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX , &totalAvailableMemory));
    GLCall(glGetIntegerv(GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX , &dedicatedGpuMemory));
    GLCall(glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX , &currentGpuMemory));

    return missing;
}

extern "C" void* CreateRenderer(void* mem , RendererArgs* args ) {
    auto r = static_cast<Renderer*>(mem);
    r->args = args;

    new(&(r->main)) Context( InitContext{nullptr,"main",1,1,4,6,1,0,1,0} );
    new(&(r->render)) Context( InitContext{&(r->main),"render",args->height,args->width,4,6,1,1,1,0} );

    if(!CheckExtensionSupport()) {
        glfwTerminate();
        return nullptr;
    }
    
    GLFWCall(glfwSetInputMode(r->render.GetContext(), GLFW_CURSOR, GLFW_CURSOR_HIDDEN ));
    r->render.SetCallBackPtr(r);
    r->render.SetWindowCloseCallBack(WindowCloseCallback);
    r->render.SetWindowSizeCallBack(WindowResizeCallBack);
    r->render.SetMousePosCallBack(MouseCallBack);
    r->render.SetCharCallBack(TextCallBack);
    r->render.SetKeyCallBack(ModKeyCallBack);

    r->render.UnBind();
    r->main.Bind();

    new(&(r->global)) Global(r);
    r->global.c.cmdBufferUpdate = false;
    new(&(r->gPass)) GeometryPass( &(r->global) );
    new(&(r->raster)) SimpleRasterPass( &r->global );
    new(&(r->text)) TextPass( &r->global );
    new(&(r->frontEnd)) ClientRendererState;

    r->shouldClose = false;
    StopRenderer((void*)r);
    new(&(r->renderThread)) std::thread( RenderLoop , r );
    
    return r;
}
extern "C" void DestroyRenderer(void* r) {
    Renderer* renderer = static_cast<Renderer*>(r);

    renderer->shouldClose = true;

    renderer->renderThread.join();
    renderer->renderThread.~thread();

    renderer->text.~TextPass();
    renderer->raster.~SimpleRasterPass();
    renderer->gPass.~GeometryPass();
    renderer->global.~Global();

    renderer->render.~Context();
    renderer->main.~Context();
}
extern "C" void* CreateContext(void* mem , void* r) {
    Renderer* renderer = static_cast<Renderer*>(r);
    auto ret = new(mem) Context( InitContext{&renderer->main,"thread shared",1,1 ,4,6 ,1,0,1,1} );
    ret->Bind();
    return ret;
}
extern "C" void DestroyContext(void* mem) {
    static_cast<Context*>(mem)->UnBind();
    static_cast<Context*>(mem)->~Context();
}

extern "C" bool RenderReady(void* renderer) {
    return !((bool)((Renderer*)renderer)->global.c.cmdBufferUpdate);
}

extern "C" void BeginRender(void* r) {
    Renderer* renderer = static_cast<Renderer*>(r);

    renderer->global.c.stateLck.lock();
    renderer->global.BeginClient();
    renderer->gPass.BeginClient();
    renderer->text.BeginClient();
    renderer->raster.BeginClient();
    renderer->global.c.stateLck.unlock();

    renderer->frontEnd.cmd = (DrawElementsIndirectCommand*)renderer->global.GetCmdBuffer().ptr;
    renderer->frontEnd.offset = 0;
}
extern "C" void EndRender(void* r) {
    Renderer* renderer = static_cast<Renderer*>(r);

    renderer->global.c.stateLck.lock();
    renderer->text.EndClient();
    renderer->raster.EndClient();
    renderer->gPass.EndClient();
    renderer->global.EndClient();
    renderer->global.c.stateLck.unlock();
}
extern "C" Mesh RegisterMesh(void* r , void* verts , uint32_t vertSize , uint32_t* indecies , uint32_t indSize , bool shadowFlag) {
    Renderer* renderer = static_cast<Renderer*>(r);

    auto& gPass = renderer->gPass;
    gPass.meshLck.lock();
   
    Mesh ret{gPass.meshVertexBuffer.GetSize() , vertSize , gPass.meshIndexBuffer.GetSize() , indSize,
             gPass.shadowVertexMesh.GetSize() , 0, gPass.shadowIndexMesh.GetSize() , 0};

    uint idVbo;
    GLCall(glGenBuffers(1,&idVbo));
    GLCall(glBindBuffer(GL_ARRAY_BUFFER , idVbo));
    GLCall(glBufferStorage(GL_ARRAY_BUFFER , std::max((uint)1,gPass.meshVertexBuffer.GetSize() + vertSize) , nullptr , GL_DYNAMIC_STORAGE_BIT));
    //GLCall(glBufferData(GL_ARRAY_BUFFER , gPass.meshVertexBuffer.GetSize() + vertSize, nullptr , GL_STATIC_DRAW ));
    GLCall(glBufferSubData(GL_ARRAY_BUFFER , gPass.meshVertexBuffer.GetSize() , vertSize , verts ));
    GLCall(glCopyNamedBufferSubData( gPass.meshVertexBuffer.GetID(), idVbo , 0,0,gPass.meshVertexBuffer.GetSize() ));

    uint idIbo;
    GLCall(glGenBuffers(1,&idIbo));
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER , idIbo));
    GLCall(glBufferStorage(GL_ELEMENT_ARRAY_BUFFER , std::max((uint)1,gPass.meshIndexBuffer.GetSize() + indSize) , nullptr , GL_DYNAMIC_STORAGE_BIT));
    //GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER , gPass.meshIndexBuffer.GetSize() + indSize, nullptr , GL_STATIC_DRAW ));
    GLCall(glBufferSubData(GL_ELEMENT_ARRAY_BUFFER , gPass.meshIndexBuffer.GetSize() , indSize , indecies ));
    GLCall(glCopyNamedBufferSubData( gPass.meshIndexBuffer.GetID(), idIbo , 0,0,gPass.meshIndexBuffer.GetSize() ));

    gPass.meshIndexBuffer.~BufferObject();
    gPass.meshVertexBuffer.~BufferObject();

    new(&gPass.meshIndexBuffer) BufferObject<GL_ELEMENT_ARRAY_BUFFER>(idIbo , gPass.meshIndexBuffer.GetSize() + indSize);
    new(&gPass.meshVertexBuffer) BufferObject<GL_ARRAY_BUFFER>(idVbo ,gPass.meshVertexBuffer.GetSize() + vertSize);

    float* sV = nullptr;
    uint* sI = nullptr;
    if( shadowFlag ) {
        MakeShadow((float*)verts ,vertSize/4 , indecies ,indSize/4, sV , sI , ret.vertSizeS, ret.indSizeS );
    }

    GLCall(glGenBuffers(1, &idVbo));
    GLCall(glBindBuffer(GL_ARRAY_BUFFER , idVbo));
    GLCall(glBufferStorage(GL_ARRAY_BUFFER , std::max((uint)1,gPass.shadowVertexMesh.GetSize() + ret.vertSizeS) , nullptr , GL_DYNAMIC_STORAGE_BIT));
    //GLCall(glBufferData(GL_ARRAY_BUFFER , gPass.shadowVertexMesh.GetSize() + ret.vertSizeS, nullptr , GL_STATIC_DRAW ));
    GLCall(glBufferSubData(GL_ARRAY_BUFFER , gPass.shadowVertexMesh.GetSize() , ret.vertSizeS , sV ));
    GLCall(glCopyNamedBufferSubData( gPass.shadowVertexMesh.GetID(), idVbo , 0,0,gPass.shadowVertexMesh.GetSize() ));

    GLCall(glGenBuffers(1, &idIbo));
    GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER , idIbo));
    GLCall(glBufferStorage(GL_ELEMENT_ARRAY_BUFFER , std::max((uint)1,gPass.shadowIndexMesh.GetSize() + ret.indSizeS) , nullptr , GL_DYNAMIC_STORAGE_BIT));
    //GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER , gPass.shadowIndexMesh.GetSize() + ret.indSizeS, nullptr , GL_STATIC_DRAW ));
    GLCall(glBufferSubData(GL_ELEMENT_ARRAY_BUFFER , gPass.shadowIndexMesh.GetSize() , ret.indSizeS , sI ));
    GLCall(glCopyNamedBufferSubData( gPass.shadowIndexMesh.GetID(), idIbo , 0,0,gPass.shadowIndexMesh.GetSize() ));

    gPass.shadowVertexMesh.~BufferObject();
    gPass.shadowIndexMesh.~BufferObject();
    new(&gPass.shadowIndexMesh) BufferObject<GL_ELEMENT_ARRAY_BUFFER>(idIbo , gPass.shadowIndexMesh.GetSize() + ret.indSizeS);
    new(&gPass.shadowVertexMesh) BufferObject<GL_ARRAY_BUFFER>(idVbo ,gPass.shadowVertexMesh.GetSize() + ret.vertSizeS);

    GLCall(glFinish());
    gPass.states[0].vaoInit = true;
    gPass.states[1].vaoInit = true;
    gPass.meshLck.unlock();

    delete[] sV;
    delete[] sI;

    return ret;
}
extern "C" void UnRegisterMesh(void* r , Mesh m) {
    //Renderer* renderer = static_cast<Renderer*>(r);
}

extern "C" void RegisterTexture(void* r , uint64_t address) {
    Renderer* renderer = static_cast<Renderer*>(r);
    renderer->global.rc.stream.streamLck.lock();

    bool b[500]{false};
    for(const auto& v : renderer->global.m.textureTranslationTable) {
        b[v.second] = true;
    }
    for(int i = 0; i < 500 ; i++) {
        if( !b[i] ) {
            renderer->global.m.textureTranslationTable[address] = i;
            renderer->global.rc.stream.stream.insert( renderer->global.rc.stream.stream.begin() ,  {address , i} );
            renderer->global.rc.stream.nonResidentOffset++;
            break;
        }
    }
    renderer->global.rc.stream.streamLck.unlock();
}
extern "C" void UnRegisterTexture(void* r , uint64_t address) {
    Renderer* renderer = static_cast<Renderer*>(r);
    renderer->global.rc.stream.streamLck.lock();
    
    renderer->global.rc.stream.stream.emplace_back( address , 0 );
    renderer->global.m.textureTranslationTable.erase(address);

    renderer->global.rc.stream.streamLck.unlock();
}
extern "C" CommonShaderParams& GetCommonRenderParam(void* r) {
    Renderer* renderer = static_cast<Renderer*>(r);
    renderer->global.c.commonShaderParamsLck.lock();
    return renderer->global.c.commonShaderParams;
}
extern "C" void ReleaseCommonRenderParam(void* r) {
    ((Renderer*)r)->global.c.commonShaderParamsLck.unlock();
}
extern "C" void Set2DTextRender(void* r , TextBox* comps, uint32_t p_size) {
    Renderer* renderer = static_cast<Renderer*>(r);
    
    renderer->frontEnd.array2d = comps;
    renderer->frontEnd.size2d = p_size;

    renderer->frontEnd.charCount2d = 0;
    renderer->frontEnd.unique.clear();
    for(int i = 0; i < renderer->frontEnd.size2d ; i++) {
        renderer->frontEnd.charCount2d += comps[i].nonSpaceCount;
        uint64_t a = comps[i].fontPtr->address;
        renderer->frontEnd.unique.emplace(a);
    }
    renderer->frontEnd.ucount2d = renderer->frontEnd.unique.size();
}
extern "C" void Set3DTextRender(void* r , Text3D* comps, uint32_t p_size) {
    Renderer* renderer = static_cast<Renderer*>(r);
    
    renderer->frontEnd.array3d = comps;
    renderer->frontEnd.size3d = p_size;
    renderer->frontEnd.charCount3d = 0;

    renderer->frontEnd.ucount3d = 0;
    for(int i = 0; i < renderer->frontEnd.size3d ; i++) {
        renderer->frontEnd.charCount3d += comps[i].nonSpaceCount;
        renderer->frontEnd.unique.insert( comps[i].fontPtr->address );
        for(int k = 0; k < renderer->frontEnd.size3d; k++ ) {
            if( (comps[i].fontPtr->address == comps[k].fontPtr->address) & (k != i) ) {
                renderer->frontEnd.ucount3d++;
            }
        }
    }
}

extern "C" void SetColoredQuads(void* r , PackedAAQuad* p_AAquads, uint32_t p_size) {
    Renderer* renderer = static_cast<Renderer*>(r);

    renderer->frontEnd.coloredAAquads = p_AAquads;
    renderer->frontEnd.coloredAAQuadSize = p_size;

}
extern "C" void SetTexturedQuads(void* r , PackedTintedTexturedAAQuad* p_texturedAAquads, uint32_t p_size) {
    Renderer* renderer = static_cast<Renderer*>(r);

    renderer->frontEnd.texturedAAQuads = p_texturedAAquads;
    renderer->frontEnd.texturedAAQuadSize = p_size;
    
}
extern "C" void SetMeshes(void* r , Renderable* arr , uint16_t p_size) {
    Renderer* renderer = static_cast<Renderer*>(r);

    renderer->frontEnd.meshes = arr;
    renderer->frontEnd.meshSize = p_size;
    renderer->frontEnd.uMeshes = 0;

    for (int i=0; i < p_size ; i++) {
        int j;
        for (j=0; j<i; j++) {
            if( arr[i].mesh.Raw() == arr[j].mesh.Raw() ) {
                break;
            }
        }
        if (i == j) {
            renderer->frontEnd.uMeshes++;
        }
    }
}

extern "C" void SetLights(void* r , ClientLight* lights , uint lightCount) {
    Renderer* renderer = static_cast<Renderer*>(r);
    renderer->frontEnd.lights = lights;
    renderer->frontEnd.lightCount = lightCount;
}

extern "C" void DrawMeshes(void* r) {
    Renderer* const renderer = static_cast<Renderer*>(r);

    auto& cmd = renderer->frontEnd.cmd;
    auto& offset = renderer->frontEnd.offset;
    auto& meshSize = renderer->frontEnd.meshSize;
    auto& meshes = renderer->frontEnd.meshes;
    auto& lightSize = renderer->frontEnd.lightCount;
    
    auto& state = renderer->gPass.GetState();

    struct GeometryObjectData {
        glm::mat4x3 transform;
        uint textureIndex;
    };
    struct ShadowObjectData {
        glm::mat4x3 transform;
    };
    struct Light {
        glm::vec4 posScale;
        glm::vec3 color;
        uint mask;
    };
    
    auto buffSize = meshSize * sizeof(GeometryObjectData) + meshSize * sizeof(ShadowObjectData) * lightSize + lightSize * sizeof(Light) + 47 + 51;
    if( buffSize > state.matrixLightBuffer.GetBuffer().GetSize()) {
        state.matrixLightBuffer.ResizeMap( buffSize * 2);
        state.vaoInit = true;
    }

    glm::vec3 camera = glm::vec3(renderer->global.c.commonShaderParams.viewPos);
    GeometryObjectData* write = (GeometryObjectData*)state.matrixLightBuffer.ptr;
    uint shadowMaskCount = 0;

    glm::mat4 projMat = renderer->global.c.commonShaderParams.projectionViewMatrix;
    glm::vec4 planes[6];
    extract_planes_from_projmat( (float(*)[4])&projMat , (float*)planes,(float*)(planes+1),(float*)(planes+2),(float*)(planes+3),(float*)(planes+4),(float*)(planes+5) );
    bool lightList[lightSize]{0};

    for(int i = 0; i < 32 ; i++) {
        state.cmd[i].size = 0;
        state.cmd[i].offset = offset;
        state.cmd[i].mask = 0;
    }
    uint lightIndex = 0;
    for(int i = 0; i < lightSize ; i++) {


        glm::vec3 p = renderer->frontEnd.lights[i].posScale;
        float radius = renderer->frontEnd.lights[i].posScale.w;

        lightList[i] = CullSphere(planes,p,radius) > 0 ? true : false;
        if( !lightList[i] ) {
            continue;
        }
        
        ((Light*)write)[lightIndex].posScale = renderer->frontEnd.lights[i].posScale;
        ((Light*)write)[lightIndex].color = renderer->frontEnd.lights[i].col;

        if( renderer->frontEnd.lights[i].shadow && (shadowMaskCount < 32) ) {
            ((Light*)write)[lightIndex].mask = 1 << shadowMaskCount;
            state.cmd[shadowMaskCount].mask = 1 << shadowMaskCount;
            state.cmd[shadowMaskCount].posScale = renderer->frontEnd.lights[i].posScale;
            shadowMaskCount++;
        }
        else {
            ((Light*)write)[lightIndex].mask = 0;
        }
        lightIndex++;
    }
    state.lightVao.cmdCount = lightIndex;
    
    Mesh m[renderer->frontEnd.uMeshes];
    float distances[meshSize];
    {
        glm::vec3 v;
        int k = 0;
        for (int i=0; i < meshSize ; i++) {

            distances[i] = glm::distance( camera , meshes[i].pos );
            int j;
            for (j=0; j<i; j++) {
                if ( meshes[i].mesh.Raw() == meshes[j].mesh.Raw()) {
                    break;
                }
            }

            if (i == j) {
                m[k] = meshes[i].mesh->mesh;
                k++;
            }
        }
    }


    uint32_t baseInstance = (lightSize * sizeof(Light) + Pad( lightSize * sizeof(Light) , sizeof(ShadowObjectData) )) / sizeof(ShadowObjectData);
    state.shadowVao.cmdCount = 0;

    for(int i = 0; i < lightSize; i++) {

        if( !renderer->frontEnd.lights[i].shadow || !lightList[i] ) {
            continue;
        }

        state.cmd[state.shadowVao.cmdCount].offset = offset;
        
        for(int j = 0; j < renderer->frontEnd.uMeshes ; j++) {

            if( m[j].indSizeS == 0 ) {
                continue;
            }
            DrawElementsIndirectCommand c{m[j].indSizeS/4 , 0 , m[j].indOffsetS/4 , m[j].vertOffsetS/12 , baseInstance };

            for(int k = 0 ; k < meshSize ; k++) {

                if( !(m[j] == meshes[k].mesh->mesh) ) {
                    continue;
                }
               
                float meshRadius = std::max(std::max(meshes[k].scale.x , meshes[k].scale.y) , meshes[k].scale.z );
                meshRadius += renderer->frontEnd.lights[i].posScale.w;
                meshRadius *= meshRadius;

                glm::vec3 aux = glm::vec3(renderer->frontEnd.lights[i].posScale) - meshes[k].pos;
                float dist = aux.x * aux.x + aux.y * aux.y + aux.z * aux.z;

                if(meshRadius < dist) {
                    continue;
                }
                
                glm::mat3 tmp = MakeTransform( meshes[k] );
                ((glm::mat4x3*)write)[baseInstance][0] = tmp[0];
                ((glm::mat4x3*)write)[baseInstance][1] = tmp[1];
                ((glm::mat4x3*)write)[baseInstance][2] = tmp[2];
                ((glm::mat4x3*)write)[baseInstance][3] = meshes[k].pos;
                baseInstance++;
                c.instanceCount++;
            }
            if( c.instanceCount != 0 ) {
                *cmd = c;
                cmd++;
                offset++;
                state.cmd[state.shadowVao.cmdCount].size++;
            }
        }
        
        if( state.cmd[state.shadowVao.cmdCount].size != 0 ) {
            state.shadowVao.cmdCount++;
            if( state.shadowVao.cmdCount > 32 ) {
                break;
            }
        }
    }

    baseInstance = (baseInstance * sizeof(ShadowObjectData) + Pad( baseInstance * sizeof(ShadowObjectData) , sizeof(GeometryObjectData) )) / sizeof(GeometryObjectData);
    state.Gvao.cmdCount = 0;
    state.Gvao.cmdOffset = offset;

    uint32_t meshSort[meshSize];
    uint32_t off = 0;
    uint32_t beg = 0;

    for(int i = 0; i < renderer->frontEnd.uMeshes; i++) {

        if( m[i].indSize == 0 ) {
            continue;
        }
        DrawElementsIndirectCommand c{m[i].indSize/4 , 0 , m[i].indOffset/4 , m[i].vertOffset/32 , baseInstance };

        beg = off;
        for(int k = 0; k < meshSize ; k++) {
            if( m[i] == meshes[k].mesh->mesh ) {
                meshSort[off] = k;
                off++;

                const auto& meshRef = meshes[k].mesh->mesh;
            }
        }

        for(int k = beg; k < off-1 ; k++) {
            
            int min_idx = k;
            for(int j = k+1 ; j < off ; j++) {

                if( distances[meshSort[j]] < distances[meshSort[min_idx]] ) {
                    min_idx = j;
                }
            }

            auto dTmp = distances[k];
            distances[k] = distances[min_idx];
            distances[min_idx] = dTmp;

            auto tmp = meshSort[k];
            meshSort[k] = meshSort[min_idx];
            meshSort[min_idx] = tmp;
        }


        auto end = renderer->global.m.textureTranslationTable.end();
        for(int k = beg ; k < off ; k++) {
            auto it = renderer->global.m.textureTranslationTable.find( meshes[meshSort[k]].tex->address );

            glm::mat3 transform = MakeTransform(meshes[meshSort[k]]);
            write[baseInstance].transform[0] = transform[0];
            write[baseInstance].transform[1] = transform[1];
            write[baseInstance].transform[2] = transform[2];
            write[baseInstance].transform[3] = meshes[meshSort[k]].pos;
            write[baseInstance].textureIndex = (it != end ? it->second : 0);
            c.instanceCount++;
            baseInstance++;
        }
        *cmd = c;
        cmd++;
        state.Gvao.cmdCount++;
        offset++;
    }

    renderer->gPass.SubmitClient();
}

extern "C" void DrawText(void* r) {
    Renderer* renderer = static_cast<Renderer*>(r);
    
    auto& charCount2d = renderer->frontEnd.charCount2d;
    auto& charCount3d = renderer->frontEnd.charCount3d;
    auto& size2d = renderer->frontEnd.size2d;
    auto& size3d = renderer->frontEnd.size3d;
    auto& unique = renderer->frontEnd.unique;
    auto& array2d = renderer->frontEnd.array2d;
    auto& array3d = renderer->frontEnd.array3d;
    auto& cmd = renderer->frontEnd.cmd;
    auto& offset = renderer->frontEnd.offset;

    auto& state = renderer->text.GetState();

    if( (charCount2d + charCount3d) * sizeof(Quad) > state.vertexBuffer.GetBuffer().GetSize() ) {
        state.vertexBuffer.ResizeMap( (charCount2d + charCount3d) * sizeof(Quad) * 2);
        state.instancedBuffer.ResizeMap( (size2d + size3d) * sizeof(uint32_t));
        state.vaoInit = true;
        const uint32_t storage = GL_MAP_WRITE_BIT;
        const uint32_t map = GL_MAP_WRITE_BIT;

        uint32_t* ibo = (uint32_t*)renderer->text.ibo.ImmutableResizeMap(nullptr , std::max(charCount2d , charCount3d) * sizeof(Quad::indecies) * 2 , storage,map );
        for(int i = 0; i < renderer->text.ibo.GetSize()/4 ; i++ ) {
            for(int k = 0; k < 6 ; k++ ) {
                ibo[i * 6 + k] = Quad::indecies[k] + i * 4;
            }
        }
        renderer->text.ibo.UnMap();
    }
    
    Quad* vbo = (Quad*)state.vertexBuffer.ptr;
    uint32_t* inst = (uint32_t*)state.instancedBuffer.ptr;


    uint32_t k = 0;
    for(const auto& u : unique ) {
        inst[k] = renderer->global.m.textureTranslationTable.at(u);
        k++;
    }

    
    k = 0;
    uint32_t counts2d[unique.size()]{0};
    uint32_t counts3d[unique.size()]{0};

    for(const auto& u : unique) {

        for(int i = 0; i < size2d ; i++ ) {
            if( array2d[i].fontPtr->address == u ) {
                counts2d[k] += array2d[i].nonSpaceCount;
                MakeTextQuads( array2d[i] , vbo );
            }
        }
        k++;
    }

    k = 0;
    for(const auto& u : unique) {
        for(int i = 0; i < size3d ; i++) {
            if( array3d[i].fontPtr->address == u) {
                counts3d[k] += array3d[i].nonSpaceCount;
                MakeTextQuads2( array3d[i] , vbo );
            }
        }
        k++;
    }


    k = 0;
    uint z = 0;
    for(uint32_t i = 0; i < unique.size() ;i++) {
        if( counts2d[i] != 0 ) {
            cmd[z] = { counts2d[i] * 6 , 1 , 0 , k , i };
            z++;
            k += counts2d[i] * 4;
        }
    }

    state.param2d.cmdCount = z;
    state.param2d.cmdOffset = offset;

    cmd += z;
    offset += z;

    z = 0;
    for(uint i = 0; i < unique.size() ; i++ ) {
        if( counts3d[i] != 0 ) {
            cmd[z] = { counts3d[i] * 6 , 1 , 0 , k , i };
            z++;
            k += counts3d[i] * 4;
        }
    }

    state.param3d.cmdCount = z;
    state.param3d.cmdOffset = offset;
    cmd += z;
    offset += z;

    renderer->text.SubmitClient();
}

extern "C" void DrawQuads(void* r) {
    Renderer* renderer = static_cast<Renderer*>(r);
    
    auto& coloredAAQuadSize = renderer->frontEnd.coloredAAQuadSize;
    auto& texturedAAQuadSize = renderer->frontEnd.texturedAAQuadSize;
    auto& coloredAAquads = renderer->frontEnd.coloredAAquads;
    auto& texturedAAQuads = renderer->frontEnd.texturedAAQuads;
    auto& cmd = renderer->frontEnd.cmd;
    auto& offset = renderer->frontEnd.offset;

    auto& state = renderer->raster.GetState();

    if( state.vbo.GetBuffer().GetSize() < sizeof(Quad) * (coloredAAQuadSize + texturedAAQuadSize)) {
        state.vbo.ResizeMap(sizeof(Quad) * (coloredAAQuadSize + texturedAAQuadSize) * 2);
        state.vaoInit = true;

        const uint32_t storage = GL_MAP_WRITE_BIT;
        const uint32_t map = GL_MAP_WRITE_BIT;

        uint32_t* ibo = (uint32_t*)renderer->raster.ibo.ImmutableResizeMap(nullptr ,sizeof(Quad::indecies) * std::max(coloredAAQuadSize ,texturedAAQuadSize) * 2,storage,map );
        for(int i = 0; i < std::max(coloredAAQuadSize ,texturedAAQuadSize) * 2 ;i++ ) {
            for(int k = 0 ; k < 6 ; k++) {
                ibo[i * 6 + k] = Quad::indecies[k] + i * 4;
            }
        }
        renderer->raster.ibo.UnMap();
    }

    void* buffer = state.vbo.ptr;
    for(int i = 0; i < coloredAAQuadSize ;i++ ) {
        MakeUniformlyColoredAAQuad( buffer , coloredAAquads[i] );
    }
    for(int i = 0; i < texturedAAQuadSize-1 ; i++) {
        auto it = renderer->global.m.textureTranslationTable.find( texturedAAQuads[i].ptr->address );
        MakeTexturedAAQuad(buffer , texturedAAQuads[i] , it->second);
    }
    MakeTexturedAAQuad(buffer , texturedAAQuads[texturedAAQuadSize-1] , 1 );
    

    *cmd = {coloredAAQuadSize * 6 , 1 , 0,0,0};
    state.coloredParam.cmdCount = 1;
    state.coloredParam.cmdOffset = offset;

    cmd++;
    offset++;

    *cmd = {texturedAAQuadSize * 6 , 1 , 0 ,coloredAAQuadSize * 3,0};
    state.texturedParam.cmdCount = 1;
    state.texturedParam.cmdOffset = offset;

    cmd++;
    offset++;

    renderer->raster.SubmitClient();
}

extern "C" void Finalize(void* r) {
    Renderer* renderer = static_cast<Renderer*>(r);

    auto s = renderer->frontEnd.ucount2d + renderer->frontEnd.ucount3d + renderer->frontEnd.uMeshes * (renderer->frontEnd.lightCount + 1);
    if( renderer->global.GetCmdBuffer().GetBuffer().GetSize() < s * sizeof(DrawElementsIndirectCommand)) {
        renderer->global.GetCmdBuffer().ResizeMap( s * 2 * sizeof(DrawElementsIndirectCommand) );
    }
}

extern "C" void PollEvents(void* r) {
    GLFWCall(glfwPollEvents());
}
extern "C" void InitTexture(Texture* texture) {
    GLCall(glGenTextures(1, (GLuint*)texture->mem ));
}
extern "C" void FreeTexture(Texture* texture) {
    GLCall(glDeleteTextures(1, (GLuint*)texture->mem ));
}
extern "C" void TextureStorage(Texture* texture , uint32_t type , uint32_t internalFormat , uint32_t mipLevels, uint32_t x,uint32_t y,uint32_t z) {
    GLCall(glBindTexture(type , *((GLuint*)texture->mem) ));

    if( type == GL_TEXTURE_2D_ARRAY ) {
        GLCall(glTexStorage3D(type, mipLevels , internalFormat , x,y,z));
    }
    else {
        GLCall(glTexStorage2D(type, mipLevels , internalFormat , x,y));
    }
}
extern "C" void TextureUpload(Texture* texture , uint32_t type , void* data , uint32_t dataFormat , uint32_t dataType , uint32_t mipLevel , uint32_t x,uint32_t y,uint32_t z) {
    GLCall(glBindTexture(type , *((GLuint*)texture->mem) ));

    const GLenum storage = GL_MAP_WRITE_BIT;
    const GLenum map = GL_MAP_WRITE_BIT;
    BufferObject<GL_PIXEL_UNPACK_BUFFER> buffer;
    if( type == GL_TEXTURE_2D_ARRAY ) {
        auto ptr = buffer.ImmutableMap(nullptr , x* y * z * 4 , storage , map );
        for(int i = 0; i < x*y*z*4 ; i++) {
            ((uint8_t*)ptr)[i] = ((uint8_t*)data)[i];
        }
        buffer.UnMap();
        GLCall(glTexSubImage3D(type , mipLevel , 0,0,0 ,x,y,z,dataFormat,dataType,0));
    }
    else {
        auto ptr = buffer.ImmutableMap(nullptr , x* y * 4 , storage , map );
        for(int i = 0; i < x*y*4 ; i++) {
            ((uint8_t*)ptr)[i] = ((uint8_t*)data)[i];
        }
        buffer.UnMap();
        GLCall(glTexSubImage2D(GL_TEXTURE_2D , mipLevel , 0,0,x,y,dataFormat,dataType,0));
    }
}
extern "C" void GenerateMipMapChain(Texture* texture) {
    GLCall(glGenerateTextureMipmap(*((GLuint*)texture->mem)));
    GLCall(glFinish());
}
extern "C" uint64_t GetTextureAddress(Texture* texture) {
    GLCall(uint64_t ret = glGetTextureHandleARB( *((GLuint*)(texture->mem)) ));
    return ret;
}
extern "C" void SetTextureParamaters(Texture* texture , uint32_t pname[] , const int32_t params[] , uint32_t size) {
    for(int i = 0; i < size ; i++) {
        GLCall(glTextureParameteri( *((GLuint*)texture->mem) , pname[i] , params[i]));
    }
}

extern "C" void SetTextureDeRefAddress( void* (*func)(void* res) ) {
    SharedResourcePtr<TextureResource>::DeRef = (TextureResource*(*)(void*)) func;
}
extern "C" void SetPBRTextureDeRefAddress( void* (*func)(void* res) ) {
    SharedResourcePtr<PBRTextureResource>::DeRef = (PBRTextureResource*(*)(void*)) func;
}
extern "C" void SetFontDeRefAddress( void* (*func)(void* res) ) {
    SharedResourcePtr<FontResource>::DeRef = (FontResource*(*)(void*))func;
}
extern "C" void SetMeshDeRefAddress( void* (*func)(void* res) ) {
    SharedResourcePtr<MeshResource>::DeRef = (MeshResource*(*)(void*))func;
}