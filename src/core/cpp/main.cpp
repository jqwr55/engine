#include <OBJ_Loader.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <Interface.h>
#include <Sound.h>
#include <Camera.h>
#include <Shadow.h>

#include <meta_info.h>
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

template< template<typename> typename T>  void DumpStructRunTime( void* basePtr ,u32 member_count , StructMemberInfoType (&info)[] ,void* user , i32 indentLevel = 0) {

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

void* RendererAllocate(void* user , u32 size) {
    return malloc(size);
}
void* RendererAlignedAllocate(void* user, u32 alignment, u32 size) {
    return aligned_alloc(alignment,size);
}
void RendererFree(void* user, void* mem) {
    free(mem);
}

i32 GetFileSize(const char* filePath) {
    auto file = fopen(filePath, "r" );
    fseek(file , 0 ,SEEK_END );
    i32 size = ftell(file);
    fclose(file);

    return size;
}

struct CommonData {
    Renderer* renderer;
    RendererInterface interface;
    RendererArgs args;
    Camera camera;
    bool open;
};

void OnResize(void* client , u32 x , u32 y) {
    CommonData* s = static_cast<CommonData*>(client);
    s->interface.SetViewPort(s->renderer , 0,0,x,y);
    s->args.windowHeight = y;
    s->args.windowWidth = x;
}
void OnClose(void* client) {
    static_cast<CommonData*>(client)->open = false;
}
void W(void* p) {
    static_cast<CommonData*>(p)->camera.keys |= 1;
}
void A(void* p) {
    static_cast<CommonData*>(p)->camera.keys |= 1 << 1;
}
void S(void* p) {
    static_cast<CommonData*>(p)->camera.keys |= 1 << 2;
}
void D(void* p) {
    static_cast<CommonData*>(p)->camera.keys |= 1 << 3;
}
void Space(void* p) {
    if(static_cast<CommonData*>(p)->camera.position.y <= 1.5) {
        static_cast<CommonData*>(p)->camera.vel += glm::vec3(0,0.05,0);
    }
}
void Shift(void* p) {
    static_cast<CommonData*>(p)->camera.keys |= 1 << 5;
}


void MousePosCallBack(void* user, double x , double y) {
    CommonData* data = static_cast<CommonData*>(user);
    float verticalAngle = (float)((data->args.windowHeight * 0.5f) - y) / (float)(data->args.windowHeight * 0.5f);
    float horizontalAngle = (float)((data->args.windowWidth * 0.5f) - x) / (float)(data->args.windowWidth * 0.5f);
    RotateCamera( data->camera , verticalAngle, -horizontalAngle);
    data->interface.SetCursorPos(data->renderer, data->args.windowWidth/2 , data->args.windowHeight/2);
}

struct Tex {
    Texture handle;
    void* data;
    u32 index;
};
struct AABB {
    glm::vec3 minBound;
    glm::vec3 maxBound;
};
struct Scene {
    std::vector<Mesh> meshes;
    std::vector<u32> textures;
    std::vector<Transform> transforms;
    std::vector<AABB> bounds;
};

void MakeLinesFromAABB(AABB box , Line* lines , glm::vec3 col) {

    glm::vec3 size = box.maxBound - box.minBound;

    lines[0].a.pos = box.minBound;
    lines[0].b.pos = box.minBound + glm::vec3(size.x , 0,0);
    lines[0].a.col = col;
    lines[0].b.col = col;

    lines[1].a.pos = box.minBound + glm::vec3(size.x,0,0);
    lines[1].b.pos = box.minBound + glm::vec3(size.x,0,size.z);
    lines[1].a.col = col;
    lines[1].b.col = col;

    lines[2].a.pos = box.minBound + glm::vec3(size.x,0,size.z);
    lines[2].b.pos = box.minBound + glm::vec3(0,0,size.z);
    lines[2].a.col = col;
    lines[2].b.col = col;

    lines[3].a.pos = box.minBound + glm::vec3(0,0,size.z);
    lines[3].b.pos = box.minBound;
    lines[3].a.col = col;
    lines[3].b.col = col;


    lines[4].a.pos = box.minBound + glm::vec3(0,size.y,0);
    lines[4].b.pos = box.minBound + glm::vec3(size.x,size.y,0);
    lines[4].a.col = col;
    lines[4].b.col = col;

    lines[5].a.pos = box.minBound + glm::vec3(size.x,size.y,0);
    lines[5].b.pos = box.minBound + glm::vec3(size.x,size.y,size.z);
    lines[5].a.col = col;
    lines[5].b.col = col;

    lines[6].a.pos = box.minBound + glm::vec3(size.x,size.y,size.z);
    lines[6].b.pos = box.minBound + glm::vec3(0,size.y,size.z);
    lines[6].a.col = col;
    lines[6].b.col = col;

    lines[7].a.pos = box.minBound + glm::vec3(0,size.y,size.z);
    lines[7].b.pos = box.minBound + glm::vec3(0,size.y,0);
    lines[7].a.col = col;
    lines[7].b.col = col;

    lines[8].a.pos = box.minBound;
    lines[8].b.pos = box.minBound + glm::vec3(0,size.y,0);
    lines[8].a.col = col;
    lines[8].b.col = col;

    lines[9].a.pos = box.minBound + glm::vec3(size.x,0,0);;
    lines[9].b.pos = box.minBound + glm::vec3(size.x,size.y,0);
    lines[9].a.col = col;
    lines[9].b.col = col;

    lines[10].a.pos = box.minBound + glm::vec3(size.x,0,size.z);;
    lines[10].b.pos = box.minBound + glm::vec3(size.x,size.y,size.z);
    lines[10].a.col = col;
    lines[10].b.col = col;

    lines[11].a.pos = box.minBound + glm::vec3(0,0,size.z);;
    lines[11].b.pos = box.minBound + glm::vec3(0,size.y,size.z);
    lines[11].a.col = col;
    lines[11].b.col = col;
}

void LoadSponza(RendererInterface* interface, Renderer* renderer , Scene& scene) {

    objl::Loader loader;
    loader.LoadFile("../res/sponza/sponza.obj");

    std::vector<Tex> textures;
    std::vector<std::string> texturePaths;

    u32 size = 0;
    for(u32 i = 0; i < loader.LoadedMeshes.size() ; i++) {

        auto Vsize = loader.LoadedMeshes[i].Vertices.size();
        auto Iptr = loader.LoadedMeshes[i].Indices.data();
        auto Isize = loader.LoadedMeshes[i].Indices.size();

        size += Vsize * sizeof(Vertex) + Isize * 4;

        Vertex verts[Vsize];

        glm::vec3 bmin,bmax;
        bmin.x = loader.LoadedMeshes[i].Vertices[0].Position.X * 0.012;
        bmin.y = loader.LoadedMeshes[i].Vertices[0].Position.Y * 0.012;
        bmin.z = loader.LoadedMeshes[i].Vertices[0].Position.Z * 0.012;

        bmax.x = loader.LoadedMeshes[i].Vertices[0].Position.X * 0.012;
        bmax.y = loader.LoadedMeshes[i].Vertices[0].Position.Y * 0.012;
        bmax.z = loader.LoadedMeshes[i].Vertices[0].Position.Z * 0.012;
        for(u32 k = 0; k < loader.LoadedMeshes[i].Vertices.size(); k++) {
            verts[k].p.x = loader.LoadedMeshes[i].Vertices[k].Position.X * 0.012;
            verts[k].p.y = loader.LoadedMeshes[i].Vertices[k].Position.Y * 0.012;
            verts[k].p.z = loader.LoadedMeshes[i].Vertices[k].Position.Z * 0.012;

            verts[k].u = loader.LoadedMeshes[i].Vertices[k].TextureCoordinate.X;
            verts[k].v = loader.LoadedMeshes[i].Vertices[k].TextureCoordinate.Y;

            bmin = glm::vec3( min<f32>(bmin.x,verts[k].p.x) , min<f32>(bmin.y,verts[k].p.y) , min<f32>(bmin.z,verts[k].p.z) );
            bmax = glm::vec3( max<f32>(bmax.x,verts[k].p.x) , max<f32>(bmax.y,verts[k].p.y) , max<f32>(bmax.z,verts[k].p.z) );
        }
        scene.bounds.push_back( {bmin,bmax} );
        scene.meshes.push_back( interface->RegisterMesh( renderer ,verts , Vsize * sizeof(Vertex),Iptr , Isize * 4 , false) );
        scene.transforms.push_back( { {0,0,0},{0,0,0},{1,1,1} } );
        
        if( loader.LoadedMeshes[i].MeshMaterial.map_Kd == "" ) {
            scene.textures.push_back(0);
            continue;
        }

        bool f2 = false;
        for(u32 k = 0; k < texturePaths.size() ; k++) {
            if( loader.LoadedMeshes[i].MeshMaterial.map_Kd == texturePaths[k] ) {
                scene.textures.push_back(textures[k].index);
                f2 = true;
                break;
            }
        }

        if(!f2) {
            texturePaths.push_back( loader.LoadedMeshes[i].MeshMaterial.map_Kd );
            Tex tex;
            i32 x,y,n;
            std::string filePath = "../res/sponza/" + loader.LoadedMeshes[i].MeshMaterial.map_Kd;

            tex.data = stbi_load( filePath.data() ,&x,&y,&n,4);
            interface->InitTexture( &(tex.handle) );
            u32 pname[2] = {0x2801, 0x2800};
            i32 params[2] = {0x2703,0x2601};
            interface->SetTextureParamaters( &(tex.handle) ,pname ,params , 2  );
            interface->TextureStorage( &(tex.handle)  , 0 , 0x8058 , 5, x,y,1);
            interface->TextureUpload(&(tex.handle) , 0 , tex.data, 0x1908 , 0x1401 , 0 ,x,y,1);
            interface->GenerateMipMapChain( &(tex.handle) );
            stbi_image_free(tex.data);
            tex.index = interface->MakeTextureResident( renderer , &(tex.handle) );


            textures.push_back(tex);
            scene.textures.push_back(tex.index);
        }

    }
    std::cout << "sponza loaded " << size << " " << textures.size() << std::endl;
}
void extract_planes_from_projmat( glm::mat4 mat, glm::vec4* left, glm::vec4* right, glm::vec4* top,glm::vec4* bottom,glm::vec4* near,glm::vec4* far) {

    for (int i = 4; i--; ) (*left)[i]      = mat[i][3] + mat[i][0];
    for (int i = 4; i--; ) (*right)[i]     = mat[i][3] - mat[i][0];
    for (int i = 4; i--; ) (*bottom)[i]    = mat[i][3] + mat[i][1];
    for (int i = 4; i--; ) (*top)[i]       = mat[i][3] - mat[i][1];
    for (int i = 4; i--; ) (*near)[i]      = mat[i][3] + mat[i][2];
    for (int i = 4; i--; ) (*far)[i]       = mat[i][3] - mat[i][2];
}

bool AABBFrustumIntersect(glm::vec4 (&frustum)[6] , AABB& box ) {

    for(u32 i = 0; i < 6 ; i++) {
        i32 out = 0;
        out += glm::dot(frustum[i] , glm::vec4(box.minBound.x,box.minBound.y,box.minBound.z, 1.f) ) < 0.0;
        out += glm::dot(frustum[i] , glm::vec4(box.maxBound.x,box.minBound.y,box.minBound.z, 1.f) ) < 0.0;
        out += glm::dot(frustum[i] , glm::vec4(box.minBound.x,box.maxBound.y,box.minBound.z, 1.f) ) < 0.0;
        out += glm::dot(frustum[i] , glm::vec4(box.maxBound.x,box.maxBound.y,box.minBound.z, 1.f) ) < 0.0;
        out += glm::dot(frustum[i] , glm::vec4(box.minBound.x,box.minBound.y,box.maxBound.z, 1.f) ) < 0.0;
        out += glm::dot(frustum[i] , glm::vec4(box.maxBound.x,box.minBound.y,box.maxBound.z, 1.f) ) < 0.0;
        out += glm::dot(frustum[i] , glm::vec4(box.minBound.x,box.maxBound.y,box.maxBound.z, 1.f) ) < 0.0;
        out += glm::dot(frustum[i] , glm::vec4(box.maxBound.x,box.maxBound.y,box.maxBound.z, 1.f) ) < 0.0;

        if(out == 8) return false;
    }
    return true;
}


extern "C" i32 maxofthree_asm(i32 a, i32 b ,i32 c);
 
int main() {


    SoundTable sounds{};
    SoundSystemState sound{};
    InitSound(sound);

    SoundAsset piano;
    LoadSoundAsset("../res/sounds/piano2.wav",&piano);

    CommonData data;
    LoadRendererInterface(data.interface , "./renderer.so");
      
    const u32 rendererSize = data.interface.SIZEOF_RENDERER();
    byte mem[rendererSize]{0};
    data.camera = { {-1,0,0} , {1,0,0} ,{0,0,0} , 0 };

    KeyCallBack callbacks[6];
    callbacks[0].user = &data;
    callbacks[0].func = W;
    callbacks[0].key = 'W';

    callbacks[1].user = &data;
    callbacks[1].func = A;
    callbacks[1].key = 'A';

    callbacks[2].user = &data;
    callbacks[2].func = S;
    callbacks[2].key = 'S';

    callbacks[3].user = &data;
    callbacks[3].func = D;
    callbacks[3].key = 'D';

    callbacks[4].user = &data;
    callbacks[4].func = Space;
    callbacks[4].key = ' ';

    callbacks[5].user = &data;
    callbacks[5].func = Shift;
    callbacks[5].key = 340;
    
    data.open = true;
    data.args.windowHeight = 1080;
    data.args.windowWidth = 1920;
    data.args.windowName = "interface test";
    data.args.Allocate = RendererAllocate;
    data.args.AllocateAligned = RendererAlignedAllocate;
    data.args.Free = RendererFree;
    data.args.NotifyClientWindowClose = OnClose;
    data.args.client = &data;
    data.args.OnCursorPos = MousePosCallBack;
    data.args.OnResize = OnResize;
    data.args.callbacks = callbacks;
    data.args.callbackCount = 6;
    Renderer* renderer = data.interface.ConstructRenderer(mem,&data.args);
    data.renderer = renderer;

    const u32 renderStateSize = data.interface.SIZEOF_RENDER_STATE();
    byte mem2[renderStateSize * 2]{0};
    RenderState* state0 = data.interface.ConstructRenderState(mem2 + renderStateSize * 0, renderer);
    RenderState* state1 = data.interface.ConstructRenderState(mem2 + renderStateSize * 1, renderer);

    Scene sponza;
    LoadSponza(&(data.interface) , renderer , sponza);

    {
    data.interface.BeginRenderState(renderer , state0);
        auto& p = *data.interface.GetShaderParams(state0);
        MemSet(&p , 0 , sizeof(p));
        p.projectionViewMatrix = glm::mat4(1);
        data.interface.ReleaseShaderParams(state0);

        data.interface.ProcessState(state0);
    data.interface.EndRenderState(renderer , state0);
    }

    {
    data.interface.BeginRenderState(renderer , state1);
        auto& p = *data.interface.GetShaderParams(state1);
        MemSet(&p , 0 , sizeof(p));
        p.projectionViewMatrix = glm::mat4(1);
        data.interface.ReleaseShaderParams(state1);

        data.interface.ProcessState(state1);
    data.interface.EndRenderState(renderer , state1);
    }
    
    data.interface.SubmitState(renderer , state0);
    data.interface.StartRenderer(renderer);

    RenderState* states[2]{state0,state1};
    i8 stateIndex = 1;
    u32 time = 0;
    glm::mat4 projection = PerspectiveMatrix(90.0f , 16.f/9.f , 0.1 ,100.0);

    

    for(u32 i = 0; i < 1 ; i++) {
        SoundInstance a;
        a.asset = &piano;
        a.currentFrame = 0;
        a.loop = true;
        a.volume = 0.05;
        PushSound(sounds , a);
    }


    Line lines[sponza.bounds.size() * 12];
    glm::vec4 frustum[6];
    u32 textures[sponza.textures.size()];
    Mesh meshes[sponza.meshes.size()];
    Transform transforms[sponza.transforms.size()];

    while(data.open) {

        if( data.interface.ReadyToRender(renderer)) {


            data.interface.BeginRenderState(renderer , states[stateIndex]);

            auto& s = *data.interface.GetShaderParams(states[stateIndex]);
            s.projectionViewMatrix = projection * LookAt( data.camera.position , data.camera.position + data.camera.direction);
            extract_planes_from_projmat(s.projectionViewMatrix , frustum,frustum+1,frustum+2,frustum+3,frustum+4,frustum+5);
            data.interface.ReleaseShaderParams(states[stateIndex]);


            u32 c = 0;
            for(u32 i = 0; i < sponza.bounds.size() ; i++) {
                if( AABBFrustumIntersect(frustum , sponza.bounds[i]) ) {
                    textures[c] = sponza.textures[i];
                    meshes[c] = sponza.meshes[i];
                    transforms[c] = sponza.transforms[i];
                    MakeLinesFromAABB(sponza.bounds[i] , lines + c * 12 , glm::vec3(1,0,0));
                    c++;
                }
            }

            // auto timer = std::chrono::high_resolution_clock::now();
            // auto end = std::chrono::high_resolution_clock::now();
            // auto deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(end-timer).count();
            // std::cout << deltaTime << " " << sponza.bounds.size() << " " << c << std::endl;

            data.interface.DrawMeshes(states[stateIndex], meshes , textures , transforms , c);
            data.interface.DrawLines(states[stateIndex], lines , c * 12);
            
            data.interface.ProcessState(states[stateIndex]);
            data.interface.EndRenderState(renderer , states[stateIndex]);
            

            data.interface.SubmitState(renderer , states[stateIndex]);

            stateIndex++;
            stateIndex = stateIndex & 1;
        }
        PlaySounds(sound,sounds);

        data.interface.PollEvents(renderer);
        data.camera.vel += glm::vec3(0,-0.0001,0);
        data.camera.position += data.camera.vel;
        if( data.camera.position.y < 1.5 ) {
            data.camera.vel.y = 0;
            data.camera.vel.x /= 1.8;
            data.camera.vel.z /= 1.8;
            data.camera.position.y = 1.5;
        } else {
            data.camera.vel.x /= 1.2;
            data.camera.vel.z /= 1.2;
        }
        MoveCameraAlong(data.camera);
        time++;
    }

    data.interface.DestroyRenderState(renderer,  state1);
    data.interface.DestroyRenderState(renderer,  state0);
    data.interface.DestroyRenderer(renderer);
    UnloadRendererInterface(data.interface);

    snd_pcm_drain(sound.handle);
	snd_pcm_close(sound.handle);
	free(sound.buffer);
    return 0;
}