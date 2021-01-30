#include <immintrin.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/scalar_multiplication.hpp>

#include <thread>
#include <cstdint>
#include <Context.h>
#include <BufferObject.h>
#include <TextureObject.h>
#include <VertexArray.h>
#include <Shader.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

struct CommonShaderParams {
    glm::mat4 projectionViewMatrix;
    glm::mat4 inverseProjectionViewMatrix;
    glm::vec4 viewUp;
    glm::vec4 viewRight;
    glm::vec4 viewDir;
    glm::vec4 viewPos;
    glm::vec4 resolutionInverseResolution;
    float TIME = 0;
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
struct RendererArgs {
    uint8_t syncInterval;
    uint16_t height;
    uint16_t width;
    uint32_t flags;
    const char* windowName;

    void* user;
    void (*MousePosCallBack)(void* user,double x ,double y);
    void (*OnClose)(void* user);
    void (*OnResize)(void* user , int w , int h);
    void (*OnText)(void* user , uint codePoint);
    void (*OnModKey)(void* user , int key, int scancode, int action, int mods);

    void* (*Allocate)(void* user , uint size);
    void* (*AlignedAllocate)(void* user, uint alignment , uint size);
    void (*Free)(void* user , void* mem);
};

union Pixel {
    struct {
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;
    };
    struct {
        uint U;
    };
};

struct Texture {
    Pixel* image;
    int x,y,n;
};

class RasterizerState {
    public:
        RasterizerState() {
            auto t = stbi_load("../res/textures/egyeb/wood.png" , &texture.x,&texture.y,&texture.n , 4);
            texture.image = (Pixel*)t;
        }
        ~RasterizerState() {}
        Texture texture;
        CommonShaderParams params;
        
};


struct SoftwareRenderer {
    RendererArgs* args;
    GLFWwindow* context;
    RasterizerState rasterizer;
    Pixel* frameBuffer;
    bool shouldClose = false;
};

void OnCLose(GLFWwindow* c) {
    GLFWCall(void* r = glfwGetWindowUserPointer(c));
    SoftwareRenderer* renderer = static_cast<SoftwareRenderer*>(r);
    renderer->shouldClose = true;
    renderer->args->OnClose(renderer->args->user);
}

void Clear(SoftwareRenderer* renderer , Pixel color) {
    uint l = (renderer->args->height * renderer->args->width);
    uint r = *((uint*)&color);
    __m128i col = _mm_set1_epi32(r);
    auto ptr = (__m128i*)renderer->frameBuffer;


    for(int i = 0; i < 518400 ; i += 8) {
        _mm_stream_si128(ptr + 0 , col);
        _mm_stream_si128(ptr + 1 , col);
        _mm_stream_si128(ptr + 2 , col);
        _mm_stream_si128(ptr + 3 , col);
        _mm_stream_si128(ptr + 4 , col);
        _mm_stream_si128(ptr + 5 , col);
        _mm_stream_si128(ptr + 6 , col);
        _mm_stream_si128(ptr + 7 , col);
        ptr += 8;
    }
}

template<typename T> T max(T t0 , T t1) {
    return t0 > t1 ? t0 : t1;
}
template<typename T> T min(T t0 , T t1) {
    return t0 < t1 ? t0 : t1;
}
float interpolate(float a, float b, float c) {
    return a *c + b * (1.0 - c);
}
struct Vertex {
    Vertex operator-(Vertex v) {
        return {x - v.x,y - v.y};
    }
    int32_t x,y;
};
struct Vertex3 {
    Vertex3 operator-(Vertex3 v) {
        return {x - v.x,y - v.y , z-v.z};
    }
    int32_t x,y;
    float z;
};
float cross2d(Vertex v0 , Vertex v1) {
    return (v0.x*v1.y - v0.y*v1.x);
}
float PointToLineDist(Vertex (&pointLine)[3] ) {
    return cross2d(pointLine[2]-pointLine[1],pointLine[0]-pointLine[1]);
}
int32_t edgeFunction(Vertex a,Vertex b, Vertex c) {
    return (a.x - c.x) * (b.y - c.y) - (a.y - c.y) * (b.x - c.x);
} 

constexpr float cubeUV[36 * 2] = {
    0.0f , 0.0f,
    1.0f , 0.0f,
    1.0f , 1.0f,

    0.0f , 1.0f,
    1.0f , 0.0f,
    1.0f , 1.0f,

    1.0f , 1.0f,
    0.0f , 0.0f,
    0.0f , 1.0f,

    0.0f , 1.0f,
    1.0f , 0.0f,
    0.0f , 0.0f,

    0.0f , 0.0f,
    0.0f , 1.0f,
    1.0f , 1.0f,
    
    0.0f , 0.0f,
    1.0f , 0.0f,
    1.0f , 1.0f,

    0.0f , 1.0f,
    1.0f , 0.0f,
    1.0f , 1.0f,

    1.0f , 1.0f,
    0.0f , 0.0f,
    0.0f , 1.0f,

    0.0f , 1.0f,
    1.0f , 0.0f,
    0.0f , 0.0f,

    0.0f , 0.0f,
    0.0f , 1.0f,
    1.0f , 1.0f,
    
    0.0f , 0.0f,
    1.0f , 0.0f,
    1.0f , 1.0f,

    0.0f , 1.0f,
    1.0f , 0.0f,
    1.0f , 1.0f,
};

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

void Transform(glm::mat4 T, glm::vec3* verts , uint vertCount, Vertex3* outVertex) {
    glm::vec4 transformed;
    for(int i = 0; i < vertCount;i++) {
        transformed = T * glm::vec4(verts[i],1);
        transformed.w = 1.0 / transformed.w;
        transformed.x *= transformed.w;
        transformed.y *= transformed.w;
        transformed.z *= transformed.w;

        transformed.x += 1;
        transformed.x *= 1920 * 0.5;
        transformed.y += 1;
        transformed.y *= 1080 * 0.5;
        // transformed.z += 1;
        // transformed.z *= 0.5;

        outVertex[i] = {transformed.x,transformed.y,transformed.z};
    }
}

void DrawRect(SoftwareRenderer* renderer , int32_t x,int32_t y , int32_t h,int32_t w , Pixel color) {

    w += x;
    h += y;

    x = max<int32_t>(x,0);
    x = min<int32_t>(x,renderer->args->width);

    y = max<int32_t>(y,0);
    y = min<int32_t>(y,renderer->args->height);;

    w = min<int32_t>(w,renderer->args->width);
    h = min<int32_t>(h,renderer->args->height);

    for(;y < h; y++) {
        for(uint k = x;k < w ; k++) {
            renderer->frameBuffer[ y * renderer->args->width + k] = color;
        }
    }
}

int32_t orient2d(Vertex a, Vertex b, Vertex c) {
    return (b.x-a.x)*(c.y-a.y) - (b.y-a.y)*(c.x-a.x);
}

void DrawTriangleFastScalar(SoftwareRenderer* renderer , Vertex3* triangle , float* UV , Texture* texture , bool show) {

    int32_t minx = min<int32_t>(triangle[0].x,triangle[1].x);
    minx = min<int32_t>(minx,triangle[2].x);
    
    int32_t miny = min<int32_t>(triangle[0].y,triangle[1].y);
    miny = min<int32_t>(miny,triangle[2].y);

    int32_t max_x = max<int32_t>(triangle[0].x,triangle[1].x);
    max_x = max<int32_t>(max_x,triangle[2].x);

    int32_t max_y = max<int32_t>(triangle[0].y,triangle[1].y);
    max_y = max<int32_t>(max_y,triangle[2].y);

    minx = max<int32_t>(minx,0);
    minx = min<int32_t>(minx,renderer->args->width);

    miny = max<int32_t>(miny,0);
    miny = min<int32_t>(miny,renderer->args->height);

    max_x = max<int32_t>(max_x,0);
    max_x = min<int32_t>(max_x,renderer->args->width);

    max_y = max<int32_t>(max_y,0);
    max_y = min<int32_t>(max_y,renderer->args->height);

    // float zCoord[3];
    // zCoord[0] = 1.0 / triangle[0].z;
    // zCoord[1] = 1.0 / triangle[1].z;
    // zCoord[2] = 1.0 / triangle[2].z;

    Vertex v[3];
    v[0] = {triangle[0].x,triangle[0].y}; // v0
    v[1] = {triangle[1].x,triangle[1].y}; // v1
    v[2] = {triangle[2].x,triangle[2].y}; // v2

    int32_t dx0 = (v[1].y - v[0].y);
    int32_t dx1 = (v[2].y - v[1].y);
    int32_t dx2 = (v[0].y - v[2].y);

    int32_t dy0 = (v[1].x - v[0].x);
    int32_t dy1 = (v[2].x - v[1].x);
    int32_t dy2 = (v[0].x - v[2].x);

    int32_t w0 = dy0 * (miny - v[0].y) - dx0 * (minx - v[0].x);
    int32_t w1 = dy1 * (miny - v[1].y) - dx1 * (minx - v[1].x);
    int32_t w2 = dy2 * (miny - v[2].y) - dx2 * (minx - v[2].x);
    //float area = 1.0 / (w0 + w1 + w2);

    int32_t r0 = dx0 * (max_x - minx) + dy0;
    int32_t r1 = dx1 * (max_x - minx) + dy1;
    int32_t r2 = dx2 * (max_x - minx) + dy2;

    //Pixel colors[3]{ {255,0,0,255},{0,255,0,255},{0,0,255,255} };
    for(;miny < max_y ; miny++) {
        for(uint x = minx ; x < max_x;x++) {
            if( (w0 | w1 | w2) >= 0 ) {
                
                // float z = 1.0 / ((w0 * zCoord[0] + w1 * zCoord[1] + w2 * zCoord[2]) * area);
                // Pixel color;
                //color.c.r = (w0 * colors[0].c.r + w1 * colors[1].c.r + w2 * colors[2].c.r) * area * z;
                //color.c.g = (w0 * colors[0].c.g + w1 * colors[1].c.g + w2 * colors[2].c.g) * area * z;
                //color.c.b = (w0 * colors[0].c.b + w1 * colors[1].c.b + w2 * colors[2].c.b) * area * z;

                // float u = (w0 * UV[0] + w1 * UV[2] + w2 * UV[4]) * area * z;
                // float v = (w0 * UV[1] + w1 * UV[3] + w2 * UV[5]) * area * z;
                // renderer->frameBuffer[x + renderer->args->width * miny] = {u*255,v*255,0,255};
                
                renderer->frameBuffer[0] = {255,255,255,255};

                //u *= texture->x;
                //v *= texture->y;
                //renderer->frameBuffer[x + renderer->args->width * miny] = texture->image[ uint(u) + uint(v * texture->x) ];
            }
            w0 -= dx0;
            w1 -= dx1;
            w2 -= dx2;

        }
        w0 += r0;
        w1 += r1;
        w2 += r2;

        if(show) {
            GLCall(glDrawPixels(renderer->args->width , renderer->args->height , GL_RGBA , GL_UNSIGNED_BYTE , renderer->frameBuffer))
            GLFWCall(glfwSwapBuffers(renderer->context));
        }
    }
}

void DrawTriangleFastSIMD(SoftwareRenderer* renderer , Vertex3* triangle , Texture* texture) {

    int32_t minx = min<int32_t>(triangle[0].x,triangle[1].x);
    minx = min<int32_t>(minx,triangle[2].x);
    
    int32_t miny = min<int32_t>(triangle[0].y,triangle[1].y);
    miny = min<int32_t>(miny,triangle[2].y);

    int32_t max_x = max<int32_t>(triangle[0].x,triangle[1].x);
    max_x = max<int32_t>(max_x,triangle[2].x);

    int32_t max_y = max<int32_t>(triangle[0].y,triangle[1].y);
    max_y = max<int32_t>(max_y,triangle[2].y);

    minx = max<int32_t>(minx,0);
    minx = min<int32_t>(minx,renderer->args->width);

    miny = max<int32_t>(miny,0);
    miny = min<int32_t>(miny,renderer->args->height);

    max_x = max<int32_t>(max_x,0);
    max_x = min<int32_t>(max_x,renderer->args->width);

    max_y = max<int32_t>(max_y,0);
    max_y = min<int32_t>(max_y,renderer->args->height);

    Vertex v[3];
    v[0] = {triangle[0].x,triangle[0].y}; // v0
    v[1] = {triangle[1].x,triangle[1].y}; // v1
    v[2] = {triangle[2].x,triangle[2].y}; // v2

    int32_t dx0 = (v[1].y - v[0].y);
    int32_t dx1 = (v[2].y - v[1].y);
    int32_t dx2 = (v[0].y - v[2].y);

    int32_t dy0 = (v[1].x - v[0].x);
    int32_t dy1 = (v[2].x - v[1].x);
    int32_t dy2 = (v[0].x - v[2].x);

    int32_t w0 = dy0 * (miny - v[0].y) - dx0 * (minx - v[0].x);
    int32_t w1 = dy1 * (miny - v[1].y) - dx1 * (minx - v[1].x);
    int32_t w2 = dy2 * (miny - v[2].y) - dx2 * (minx - v[2].x);

    //int32_t r0 = dx0 * (max_x - minx) + dy0;
    //int32_t r1 = dx1 * (max_x - minx) + dy1;
    //int32_t r2 = dx2 * (max_x - minx) + dy2;

    uint32_t inner = (max_x - minx) & 3;
    max_x -= inner;
    int32_t itNum = max_x - minx;
    
    __m128i _r0 = _mm_set1_epi32(dx0 * (itNum) + dy0);
    __m128i _r1 = _mm_set1_epi32(dx1 * (itNum) + dy1);
    __m128i _r2 = _mm_set1_epi32(dx2 * (itNum) + dy2);

    __m128i _w0 = _mm_setr_epi32(w0 , w0 - (dx0 * 1), w0 - (dx0 * 2) , w0 - (dx0 * 3) );
    __m128i _w1 = _mm_setr_epi32(w1 , w1 - (dx1 * 1), w1 - (dx1 * 2) , w1 - (dx1 * 3) );
    __m128i _w2 = _mm_setr_epi32(w2 , w2 - (dx2 * 1), w2 - (dx2 * 2) , w2 - (dx2 * 3) );

    __m128i _dx0 = _mm_set1_epi32(dx0 * 4);
    __m128i _dx1 = _mm_set1_epi32(dx1 * 4);
    __m128i _dx2 = _mm_set1_epi32(dx2 * 4);
    
    for(;miny < max_y ; miny++) {

        for(uint x = minx; x < max_x;x+=4) {

            __m128i _mask = _mm_or_si128(_w0 , _w1);
            _mask = _mm_or_si128(_mask , _w2);
            _mask = _mm_cmplt_epi32(_mask , _mm_set1_epi32(0) );
            _mask = _mm_xor_si128(_mask , _mm_set1_epi32(0xFFFFFFFF) );
            int allFail = _mm_test_all_zeros(_mask , _mm_set1_epi32(0xFFFFFFFF));

            if( !allFail ) {
                //_mm_maskmoveu_si128( _mm_set1_epi32(Pixel{255,255,255,255}.u), _mask , (char*)(renderer->frameBuffer + x + miny * renderer->args->width) );
            }

            _w0 = _mm_sub_epi32(_w0 , _dx0);
            _w1 = _mm_sub_epi32(_w1 , _dx1);
            _w2 = _mm_sub_epi32(_w2 , _dx2);
        }

        if( inner != 0 ) {
            __m128i _mask = _mm_or_si128(_w0 , _w1);
            _mask = _mm_or_si128(_mask , _w2);
            _mask = _mm_cmplt_epi32(_mask , _mm_set1_epi32(0) );
            _mask = _mm_xor_si128(_mask , _mm_set1_epi32(0xFFFFFFFF) );

            // for(int i = 0; i < inner ; i++) {
            //    if( ((int32_t*)&_mask)[i] ) {
            //        renderer->frameBuffer[max_x + i + miny * renderer->args->width] = {255,255,255,255};
            //     }
            // }
        
            ((int32_t*)&_mask)[1] &= inner > 1 ? 0xFFFFFFFF : 0;
            ((int32_t*)&_mask)[2] &= inner > 2 ? 0xFFFFFFFF : 0;
            ((int32_t*)&_mask)[3] = 0;
            //_mm_maskmoveu_si128( _mm_set1_epi32(Pixel{255,255,255,255}.u), _mask , (char*)(renderer->frameBuffer + max_x + miny * renderer->args->width) );
        }

        _w0 = _mm_add_epi32(_w0,_r0);
        _w1 = _mm_add_epi32(_w1,_r1);
        _w2 = _mm_add_epi32(_w2,_r2);
    }
}

void DrawTriangle(SoftwareRenderer* renderer , Vertex3 (&triangle)[3] , Pixel color , Texture* texture ) {


    int32_t minx = min<int32_t>(triangle[0].x,triangle[1].x);
    minx = min<int32_t>(minx,triangle[2].x);
    
    int32_t miny = min<int32_t>(triangle[0].y,triangle[1].y);
    miny = min<int32_t>(miny,triangle[2].y);

    int32_t max_x = max<int32_t>(triangle[0].x,triangle[1].x);
    max_x = max<int32_t>(max_x,triangle[2].x);

    int32_t max_y = max<int32_t>(triangle[0].y,triangle[1].y);
    max_y = max<int32_t>(max_y,triangle[2].y);

    minx = max<int32_t>(minx,0);
    minx = min<int32_t>(minx,renderer->args->width);

    miny = max<int32_t>(miny,0);
    miny = min<int32_t>(miny,renderer->args->height);

    max_x = max<int32_t>(max_x,0);
    max_x = min<int32_t>(max_x,renderer->args->width);

    max_y = max<int32_t>(max_y,0);
    max_y = min<int32_t>(max_y,renderer->args->height);

    Vertex tri[3];
    tri[0] = {triangle[0].x,triangle[0].y};
    tri[1] = {triangle[1].x,triangle[1].y};
    tri[2] = {triangle[2].x,triangle[2].y};

    float du = float(texture->x) / float(max_x - minx); // [0, texture->x] [y,h]
    float dv = float(texture->y) / float(max_y - miny); // [0, texture->v] [k,w]

    float v = 0;
    for(;miny < max_y;miny++) {
        uint index = renderer->args->width * miny;
        float u = 0;
        for(int32_t x = minx;x < max_x;x++) {

            int32_t t0 = edgeFunction(tri[1] ,tri[2] , {x,miny} );
            if( t0 < 0 ) {
                continue;
            }
            int32_t t1 = edgeFunction(tri[2] ,tri[0] , {x,miny} );
            if( t1 < 0 ) {
                continue;
            }
            int32_t t2 = edgeFunction(tri[0] ,tri[1] , {x,miny} );
            if( t2 < 0 ) {
                continue;
            }

            //renderer->frameBuffer[renderer->args->width * miny + x] = color;
            renderer->frameBuffer[index + x] = texture->image[ uint(u) + uint(v) * texture->x ];
            u += du;
        }
        v += dv;
    }
}

void DrawRectBitmap(SoftwareRenderer* renderer , int32_t x , int32_t y , int32_t h , int32_t w , Texture* texture) {

    float du = float(texture->x) / float(w); // [0, texture->x] [y,h]
    float dv = float(texture->y) / float(h); // [0, texture->v] [k,w]

    w += x;
    h += y;

    x = max<int32_t>(x,0);
    x = min<int32_t>(x,renderer->args->width);

    y = max<int32_t>(y,0);
    y = min<int32_t>(y,renderer->args->height);

    w = min<int32_t>(w,renderer->args->width);
    h = min<int32_t>(h,renderer->args->height);
  
    float v = 0;
    for(;y < h; y++) {
        uint index = y * renderer->args->width;
        float u = 0;
        for(uint k = x;k < w ; k+= 1) {
            u += du;
            renderer->frameBuffer[index + k] = texture->image[ uint(u) + uint(v) * texture->x ];
        }
        v += dv;
    }
}

void DrawRectFancy(SoftwareRenderer* renderer , int32_t x , int32_t y , int32_t h , int32_t w , Pixel (&colors)[4] ) {
    
    w += x;
    h += y;

    x = max<int32_t>(x,0);
    x = min<int32_t>(x,renderer->args->width);

    y = max<int32_t>(y,0);
    y = min<int32_t>(y,renderer->args->height);;

    w = min<int32_t>(w,renderer->args->width);
    h = min<int32_t>(h,renderer->args->height);

    uint dxI = 16777215 / (w-x);
    uint dyI = 16777215 / (h-y);
    uint ay = 0;

    int d0r = (8388608 * (int(colors[3].r) - int(colors[0].r)) ) / (w-x);
    int d0g = (8388608 * (int(colors[3].g) - int(colors[0].g)) ) / (w-x);
    int d0b = (8388608 * (int(colors[3].b) - int(colors[0].b)) ) / (w-x);
    int d0a = (8388608 * (int(colors[3].a) - int(colors[0].a)) ) / (w-x);

    int d1r = (8388608 * (int(colors[2].r) - int(colors[1].r)) ) / (w-x);
    int d1g = (8388608 * (int(colors[2].g) - int(colors[1].g)) ) / (w-x);
    int d1b = (8388608 * (int(colors[2].b) - int(colors[1].b)) ) / (w-x);
    int d1a = (8388608 * (int(colors[2].a) - int(colors[1].a)) ) / (w-x);
    
    for(;y < h; y++) {

        //uint ax = 0;
        uint by = 16777216 - ay;

        uint r0 = colors[0].r << 23;
        uint g0 = colors[0].g << 23;
        uint b0 = colors[0].b << 23;
        uint a0 = colors[0].a << 23;

        uint r1 = colors[1].r << 23;
        uint g1 = colors[1].g << 23;
        uint b1 = colors[1].b << 23;
        uint a1 = colors[1].a << 23;

        uint q0r = (ay * (r1 >> 23) + by * (r0 >> 23) ) >> 24;
        uint q0g = (ay * (g1 >> 23) + by * (g0 >> 23) ) >> 24;
        uint q0b = (ay * (b1 >> 23) + by * (b0 >> 23) ) >> 24;
        uint q0a = (ay * (a1 >> 23) + by * (a0 >> 23) ) >> 24;

        r0 += d0r * (w-x-1);
        g0 += d0g * (w-x-1);
        b0 += d0b * (w-x-1);
        a0 += d0a * (w-x-1);

        r1 += d1r * (w-x-1);
        g1 += d1g * (w-x-1);
        b1 += d1b * (w-x-1);
        a1 += d1a * (w-x-1);

        uint q1r = (ay * (r1 >> 23) + by * (r0 >> 23) ) >> 24;
        uint q1g = (ay * (g1 >> 23) + by * (g0 >> 23) ) >> 24;
        uint q1b = (ay * (b1 >> 23) + by * (b0 >> 23) ) >> 24;
        uint q1a = (ay * (a1 >> 23) + by * (a0 >> 23) ) >> 24;

        int dqr = ( ( int(q1r) - int(q0r) ) << 23 ) / (w-x);
        int dqg = ( ( int(q1g) - int(q0g) ) << 23 ) / (w-x);
        int dqb = ( ( int(q1b) - int(q0b) ) << 23 ) / (w-x);
        int dqa = ( ( int(q1a) - int(q0a) ) << 23 ) / (w-x);

        uint r = q0r << 23;
        uint g = q0g << 23;
        uint b = q0b << 23;
        uint a = q0a << 23;

        uint index = y * renderer->args->width;
        for(uint k = x;k < w ; k+= 1) {

            Pixel finalColor;
            r += dqr;
            g += dqg;
            b += dqb;
            a += dqa;
            finalColor.r = r >> 23;
            finalColor.g = g >> 23;
            finalColor.b = b >> 23;
            finalColor.a = a >> 23;
            renderer->frameBuffer[k + index] = finalColor;
        }
        ay += dyI;
    }
}

void MouseCallBack(GLFWwindow* w, double x ,double y) {
    GLFWCall(SoftwareRenderer* renderer = static_cast<SoftwareRenderer*>(glfwGetWindowUserPointer(w)));
    renderer->args->MousePosCallBack( renderer->args->user , x ,y );
}

extern "C" const uint32_t SIZEOF_CONTEXT() {
    return sizeof(Context);
}
extern "C" const uint32_t SIZEOF_RENDERER() {
    return sizeof(SoftwareRenderer);
}
extern "C" bool ShouldClose(void* r) {
    SoftwareRenderer* renderer = static_cast<SoftwareRenderer*>(r);
    return renderer->shouldClose;
}
extern "C" void* CreateRenderer(void* mem , RendererArgs* args ) {
    SoftwareRenderer* r = static_cast<SoftwareRenderer*>(mem);
    r->args = args;
    r->frameBuffer = (Pixel*)args->AlignedAllocate(args->user , 64 , args->width * args->height * sizeof(Pixel) );
    
    Clear(r , Pixel{0,0,0,0});
    glfwInit();
    GLFWCall(r->context = glfwCreateWindow(args->width, args->height , args->windowName , NULL  , NULL ));
    GLFWCall(glfwMakeContextCurrent(0));
    GLFWCall(glfwMakeContextCurrent(r->context));
    GLFWCall(glfwSwapInterval(1));
    Context::InitGLEW();
    new(&(r->rasterizer)) RasterizerState;

    GLFWCall(glfwSetWindowUserPointer(r->context , mem));
    GLFWCall(glfwSetWindowCloseCallback( r->context , OnCLose ));
    GLFWCall(glfwSetCursorPosCallback(r->context, MouseCallBack));

    r->shouldClose = false;
    GLFWCall(glfwSwapInterval(1));
    
    return mem;
}
extern "C" void DestroyRenderer(void* r) {
    SoftwareRenderer* renderer = static_cast<SoftwareRenderer*>(r);
    renderer->shouldClose = true;

    renderer->rasterizer.~RasterizerState();
    renderer->args->Free( renderer->args->user , renderer->frameBuffer );
}
extern "C" bool RenderReady(void* renderer) {
    return true;
}
extern "C" void BeginRender(void* r) {}
extern "C" CommonShaderParams& GetCommonRenderParam(void* r) {
    SoftwareRenderer* renderer = static_cast<SoftwareRenderer*>(r);
    return renderer->rasterizer.params;
}
extern "C" void Check(void* r) {
    SoftwareRenderer* renderer = static_cast<SoftwareRenderer*>(r);
    std::cout << (uint64_t)renderer->frameBuffer << std::endl;
}
extern "C" void DrawMeshes(void* r) {
    SoftwareRenderer* renderer = static_cast<SoftwareRenderer*>(r);

    auto timer = std::chrono::high_resolution_clock::now();

    Clear(renderer , Pixel{0,0,0,255});

    static bool show = false;
    if(glfwGetKey(renderer->context , GLFW_KEY_SPACE) == GLFW_PRESS) {
        show = !show;
    }

    Vertex3 cubeT[36];
    Transform(renderer->rasterizer.params.projectionViewMatrix , (glm::vec3*)cube ,36, cubeT );
    for(int i = 0; i < 12 ; i++) {

        Vertex3 triangle[3];
        triangle[0] = cubeT[i * 3 + 0];
        triangle[1] = cubeT[i * 3 + 1];
        triangle[2] = cubeT[i * 3 + 2];

        // float uvs[6];
        // uvs[0] = cubeUV[i * 6 + 0] * 0.1;
        // uvs[1] = cubeUV[i * 6 + 1] * 0.1;

        // uvs[2] = cubeUV[i * 6 + 2] * 0.1;
        // uvs[3] = cubeUV[i * 6 + 3] * 0.1;

        // uvs[4] = cubeUV[i * 6 + 4] * 0.1;
        // uvs[5] = cubeUV[i * 6 + 5] * 0.1;

        // if(orient2d( {triangle[0].x,triangle[0].y} , {triangle[1].x,triangle[1].y} , {triangle[2].x,triangle[2].y}) < 0 ) {
        //     triangle[0] = cubeT[i * 3 + 0];
        //     triangle[1] = cubeT[i * 3 + 1];
        //     triangle[2] = cubeT[i * 3 + 2];

        //     uvs[0] = cubeUV[i * 6 + 4];
        //     uvs[1] = cubeUV[i * 6 + 5];

        //     uvs[2] = cubeUV[i * 6 + 2];
        //     uvs[3] = cubeUV[i * 6 + 3];

        //     uvs[4] = cubeUV[i * 6 + 0];
        //     uvs[5] = cubeUV[i * 6 + 1];
        // }

        #if 0
        DrawTriangleFastSIMD(renderer , triangle , nullptr);
        #else
        DrawTriangleFastScalar(renderer , triangle , nullptr , &renderer->rasterizer.texture , show);
        #endif
        //DrawRect(renderer , triangle[0].x , triangle[0].y , 10,10 , {255,255,255,255});
        //DrawRect(renderer , triangle[1].x , triangle[1].y , 10,10 , {255,255,255,255});
        //DrawRect(renderer , triangle[2].x , triangle[2].y , 10,10 , {255,255,255,255});
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(end-timer).count();
    std::cout << deltaTime << std::endl;

    GLCall(glDrawPixels(renderer->args->width , renderer->args->height , GL_RGBA , GL_UNSIGNED_BYTE , renderer->frameBuffer))
    GLFWCall(glfwSwapBuffers(renderer->context));

    std::this_thread::sleep_for( std::chrono::milliseconds(30) );
}

extern "C" bool IsKeyPressed(void* r , int key) {
    SoftwareRenderer* renderer = static_cast<SoftwareRenderer*>(r);
    GLFWCall(int ret = glfwGetKey(renderer->context , key));
    return ret;
}
extern "C" void StartRenderer(void* r) {}
extern "C" void StopRenderer(void* r) {}
extern "C" void CursorVisibility(void* r, bool a) {}
extern "C" void Show(void* r) {}
extern "C" void Hide(void* r) {}
extern "C" void Resize(void* r, uint x , uint y) {}
extern "C" void Close(void* r) {}
extern "C" void Open(void* r) {}
extern "C" void SetCursorPos(void* r , uint x, uint y) {
    SoftwareRenderer* renderer = static_cast<SoftwareRenderer*>(r);
    GLFWCall(glfwSetCursorPos(renderer->context , x,y));
}

extern "C" void ReleaseCommonRenderParam(void* r) {}
extern "C" void* CreateContext(void* mem , void* r) {return nullptr;}
extern "C" void DestroyContext(void* mem) {}
extern "C" void EndRender(void* r) {}
extern "C" Mesh RegisterMesh(void* r , void* verts , uint32_t vertSize , uint32_t* indecies , uint32_t indSize , bool shadowFlag) {
    return {0};
}
extern "C" void UnRegisterMesh(void* r , Mesh m) {}
extern "C" void RegisterTexture(void* r , uint64_t address) {}
extern "C" void UnRegisterTexture(void* r , uint64_t address) {}
extern "C" void SetLights(void* r , void* lights , uint lightCount) {}
extern "C" void Set2DTextRender(void* r , void* comps, uint32_t p_size) {}
extern "C" void Set3DTextRender(void* r , void* comps, uint32_t p_size) {}
extern "C" void SetColoredQuads(void* r , void* p_AAquads, uint32_t p_size) {}
extern "C" void SetTexturedQuads(void* r , void* p_texturedAAquads, uint32_t p_size) {}
extern "C" void SetMeshes(void* r , void* arr , uint16_t p_size) {}
extern "C" void DrawText(void* r) {}
extern "C" void DrawQuads(void* r) {}
extern "C" void Finalize(void* r) {}
extern "C" void PollEvents(void* r) {glfwPollEvents();}
extern "C" void InitTexture(void* texture) {}
extern "C" void FreeTexture(void* texture) {}
extern "C" void TextureStorage(void* texture , uint32_t type , uint32_t internalFormat , uint32_t mipLevels, uint32_t x,uint32_t y,uint32_t z) {}
extern "C" void TextureUpload(void* texture , uint32_t type , void* data , uint32_t dataFormat , uint32_t dataType , uint32_t mipLevel , uint32_t x,uint32_t y,uint32_t z) {}
extern "C" void GenerateMipMapChain(void* texture) {}
extern "C" uint64_t GetTextureAddress(void* texture) {return 0;}
extern "C" void SetTextureParamaters(void* texture , uint32_t pname[] , const int32_t params[] , uint32_t size) {}
extern "C" void SetTextureDeRefAddress( void* (*func)(void* res) ) {}
extern "C" void SetPBRTextureDeRefAddress( void* (*func)(void* res) ) {}
extern "C" void SetFontDeRefAddress( void* (*func)(void* res) ) {}
extern "C" void SetMeshDeRefAddress( void* (*func)(void* res) ) {}