#include <glm/gtx/euler_angles.hpp>
#include <OBJ_Loader.h>
#include <InterFaceFunctions.h>

void DrawLine(glm::vec2 p0 , glm::vec2 p1 , glm::vec3 color) {

    glUniform3f(2 , color.x,color.y,color.z);

    float l = glm::distance(p0,p1);
    glm::vec2 dir = glm::normalize(p1-p0);

    const int pCount = 500;

    for(int i = 0 ; i < pCount ; i++) {
        auto p = p0 + (dir * l * ((float)i / (float)pCount ));
        glUniform2f(1 , p.x , p.y );
        GLCall(glDrawArrays(GL_POINTS , 0 , 1));
    }
}
glm::vec4 DrawEllispeBoundingBox(glm::vec2 c , glm::vec2 r , float angle) {
    float rX = r.x;
    float rY = r.y;

    float rXSquared = rX*rX;
    float rYSquared = rY*rY;
    float sinAngleSquared = sin(angle);
    sinAngleSquared *= sinAngleSquared;
    float cosAngleSquared = cos(angle);
    cosAngleSquared *= cosAngleSquared;

    float xDim = sqrt( rXSquared * cosAngleSquared + rYSquared * sinAngleSquared ) / (2.5f);
    float yDim = sqrt( rXSquared * sinAngleSquared + rYSquared * cosAngleSquared ) / (2.5f);

    auto lLeft = glm::vec2(c.x - (xDim/2.f) ,c.y -(yDim/2) );
    auto uLeft = lLeft + glm::vec2(0,yDim);
    auto uRight = lLeft + glm::vec2(xDim ,yDim);
    auto lRight = lLeft + glm::vec2(xDim ,0);

    DrawLine(lLeft , uLeft , glm::vec3(1,0,0) );
    DrawLine(uLeft , uRight, glm::vec3(1,0,0) );
    DrawLine(uRight ,lRight, glm::vec3(1,0,0) );
    DrawLine(lRight , lLeft , glm::vec3(1,0,0) );

    return { lLeft.x , lLeft.y , xDim , yDim };
}

glm::mat4 PerspectiveMatrix(float fov , float aspect , float near , float far) {

    float x = 1 / (aspect * tan(fov/2));
    float y = 1 / ( tan(fov / 2 ) );
    float z = -(far + near) / (far - near);
    float w = (-2 * far * near) / (far - near);

    return glm::mat4{
        x,0,0,0,
        0,y,0,0,
        0,0,z,-1,
        0,0,w,0
    };
}
glm::mat4 LookAt(glm::vec3 from , glm::vec3 to , glm::vec3 worldUp = {0,1,0} ) {
    glm::vec3 forward{ glm::normalize(to - from) };
    glm::vec3 right{glm::normalize(glm::cross( forward , worldUp ))};
    glm::vec3 up{glm::normalize(glm::cross(right , forward))};
    
    return glm::mat4{
       right.x , up.x , -forward.x , 0,
       right.y , up.y , -forward.y , 0,
       right.z , up.z , -forward.z , 0,
       -glm::dot(right , from),-glm::dot(up , from),glm::dot(forward , from),1
    };
}

void GetBoundsForAxis(glm::vec3 axis , glm::vec3 C , float r , float nearZ , glm::vec3& L , glm::vec3& U) {

    glm::vec2 c = glm::vec2(glm::dot( axis , C  ) , C.z );
    glm::vec2 bounds[2];

    float tSquared = glm::dot(c,c) - r*r;
    bool cameraInsideSphere = tSquared <= 0;

    glm::vec2 v = cameraInsideSphere ? glm::vec2(0) : glm::vec2( sqrt(tSquared) ,r  ) / glm::length(c);
    bool clipSphere = c.y + r >= nearZ;
    float k = sqrt( r*r - ((nearZ - c.y) * (nearZ - c.y)) );

    for(int i = 0 ; i < 2 ; i++) {
        if( !cameraInsideSphere ) {
            bounds[i] = glm::mat2( v.x , -v.y , v.y , v.x ) * c * v.x;
        }
        bool clipBound = cameraInsideSphere || (bounds[i].y > nearZ );
        if( clipSphere && clipBound ) {
            bounds[i] = glm::vec2( k , nearZ );
        }

        v.y = -v.y;
        k = -k;
    }

    L = bounds[1].x * axis;
    L.z = bounds[1].y;
    U = bounds[0].x * axis;
    U.z = bounds[0].y;
}

glm::vec4 GetAABB( glm::vec3 C , float r , float nearZ , const glm::mat4& P ) {
    glm::vec3 right , left , top , bottom;

    GetBoundsForAxis( glm::vec3(1,0,0) , C ,r ,nearZ , right , left  );
    GetBoundsForAxis( glm::vec3(0,1,0) , C ,r ,nearZ , top   , bottom);

    glm::vec4 x = P * glm::vec4( left , 1 );
    x /= x.w;
    glm::vec4 y = P * glm::vec4( bottom , 1 );
    y /= y.w;
    glm::vec4 z = P * glm::vec4( right , 1 );
    z /= z.w;
    glm::vec4 w = P * glm::vec4( top , 1 );
    w /= w.w;

    return { x.x , y.y , z.x , w.y };
}

void GetLightAABB(glm::vec4 sphere , CommonShaderParams& params , glm::vec4& bounds ) {

    glm::mat4 p = params.projectionViewMatrix;
    glm::vec3 dir = params.viewDir;
    glm::vec3 pos = params.viewPos;

    glm::mat4 view = LookAt( pos , pos + dir );
    glm::vec4 viewSphere = view * glm::vec4( glm::vec3(sphere) , 1 );

    auto b = GetAABB( glm::vec3(viewSphere) , sphere.w , -0.1f , PerspectiveMatrix(90.f , (float)1920/(float)1080 , 0.1f , 100.f) );

    float x = b.x;
    float y = b.y;
    float xS = b.z;
    float yS = b.w;

    if( !( (x <= xS) | (y <= yS) ) ) {
        bounds = glm::vec4(-1,-1,1,1);
        return;
    }

    x = std::max( std::min(x , 1.0f) ,-1.0f);
    y = std::max( std::min(y , 1.0f) ,-1.0f);
    xS = std::max( std::min(xS , 1.0f) ,-1.0f);
    yS = std::max( std::min(yS , 1.0f) ,-1.0f);

    auto d = pos - glm::vec3(sphere);
    float D = glm::dot(d,d);

    bool insideCamera = (sphere.w*sphere.w - D) > 0.0;
    x = insideCamera ? -1 : x;
    y = insideCamera ? -1 : y;
    xS = insideCamera ? 1 : xS;
    yS = insideCamera ? 1 : yS;

    bounds = glm::vec4(x,y,xS,yS);
}
void extract_planes_from_projmat(
        const float mat[4][4],
        float left[4], float right[4], float top[4], float bottom[4],
        float near[4], float far[4])
{
    for (int i = 4; i--; ) left[i]      = mat[i][3] + mat[i][0];
    for (int i = 4; i--; ) right[i]     = mat[i][3] - mat[i][0];
    for (int i = 4; i--; ) bottom[i]    = mat[i][3] + mat[i][1];
    for (int i = 4; i--; ) top[i]       = mat[i][3] - mat[i][1];
    for (int i = 4; i--; ) near[i]      = mat[i][3] + mat[i][2];
    for (int i = 4; i--; ) far[i]       = mat[i][3] - mat[i][2];
}
float DistanceToPlane( glm::vec4 vPlane, glm::vec3 vPoint ) {
    return glm::dot(glm::vec4(vPoint, 1.0), vPlane);
}
float CullSphere( glm::vec4 vPlanes[6], glm::vec3 vCenter, float fRadius ) {
   float dist01 = glm::min(DistanceToPlane(vPlanes[0], vCenter), DistanceToPlane(vPlanes[1], vCenter));
   float dist23 = glm::min(DistanceToPlane(vPlanes[2], vCenter), DistanceToPlane(vPlanes[3], vCenter));
   float dist45 = glm::min(DistanceToPlane(vPlanes[4], vCenter), DistanceToPlane(vPlanes[5], vCenter));
 
   return glm::min(glm::min(dist01, dist23), dist45) + fRadius;
}
bool Any_4x32(glm::vec<4,uint32_t> c) {
    return bool(c[0] | c[1] | c[2] | c[3]);
}
bool All_4x32(glm::vec<4,uint32_t> c) {
    return bool(c[0] & c[1] & c[2] & c[3]);
}
glm::vec<4,uint32_t> LessThan(glm::vec4 a , glm::vec4 b) {
    return { a[0]<b[0] , a[1]<b[1] , a[2]<b[2] , a[3]<b[3] };
}
glm::vec4 exp_4x32(glm::vec4 x) {
    return { exp(x[0]),exp(x[1]),exp(x[2]),exp(x[3]) };
}
glm::vec4 tan_4x32(glm::vec4 x) {
    return { tan(x[0]),tan(x[1]),tan(x[2]),tan(x[3]) };
}
glm::vec4 F_4x32(glm::vec4 u , glm::vec4 v, float steps) {

    glm::vec4 dx = v / steps;
    glm::vec4 x(0.0f);
    glm::vec4 res(0.0f);

    while( Any_4x32( LessThan(x,v) ) ) {
        res += exp_4x32( -u * tan_4x32(x) ) * dx;
        x += dx;
    }

    return res;
}
float F(float u , float v, int steps) {

    float dx = v/(float)steps;
    float x = 0;
    float res = 0;

    while( x < v ) {
        res += exp( -u*tan(x) ) * dx;
        x += dx;
    }

    return res;
}
void MakeFuncLookupTable(float* tex , int size , float steps) {

    glm::vec4 u,v;

    for(int i = 0; i < size ; i++) {
        for(int k = 0; k < size ; k += 4) {
            
            u = glm::vec4(((float)i/(float)size) * 10);
            v[0] = (((float)(k + 0)) / (float)size) * 1.57079633f;
            v[1] = (((float)(k + 1)) / (float)size) * 1.57079633f;
            v[2] = (((float)(k + 2)) / (float)size) * 1.57079633f;
            v[3] = (((float)(k + 3)) / (float)size) * 1.57079633f;
            glm::vec4 height = F_4x32( u,v, 1000.0f );
            tex[i * size + k + 0] = height[0];
            tex[i * size + k + 1] = height[1];
            tex[i * size + k + 2] = height[2];
            tex[i * size + k + 3] = height[3];
        }
    }
}

struct hash_pair {
   template <class T1, class T2>
   size_t operator()(const std::pair<T1, T2>& pair) const{
      auto hash1 = std::hash<T1>{}(pair.first);
      auto hash2 = std::hash<T2>{}(pair.second);
      return hash1 ^ hash2;
   }
};

void FindAdjecencies(const std::vector<unsigned int>& indices , std::vector<unsigned int>& adjacentindices) {
    
    adjacentindices.reserve(indices.size() * 2);
    std::unordered_map<std::pair<unsigned int , unsigned int>,unsigned int , hash_pair> map;
    std::pair<unsigned int, unsigned int>pair;

    for(int i = 0 ; i < indices.size() / 3 ; i++) {
        pair.first = indices[i * 3 + 0];
        pair.second = indices[i * 3 + 1];
        map[pair] = indices[i * 3 + 2];

        pair.first = indices[i * 3 + 1];
        pair.second = indices[i * 3 + 2];
        map[pair] = indices[i * 3 + 0];

        pair.first = indices[i * 3 + 2];
        pair.second = indices[i * 3 + 0];
        map[pair] = indices[i * 3 + 1];
    }

    for(int i = 0; i < indices.size() / 3 ; i++) {
        pair.first = indices[i * 3 + 1];
        pair.second = indices[i * 3 + 0];

        auto a = map.find(pair);
        if(a != map.end()) {
            adjacentindices.push_back(pair.second);
            adjacentindices.push_back(map[pair]);
        }
        else {
            std::cout << "no adjacency" << std::endl;
            adjacentindices.clear();
            return;
        }

        pair.first = indices[i * 3 + 2];
        pair.second = indices[i * 3 + 1];

        a = map.find(pair);
        if(a != map.end()) {
            adjacentindices.push_back(pair.second);
            adjacentindices.push_back(map[pair]);
        }
        else {
            std::cout << "no adjacency" << std::endl;
            adjacentindices.clear();
            return;
        }

        pair.first = indices[i * 3 + 0];
        pair.second = indices[i * 3 + 2];

        a = map.find(pair);
        if(a != map.end()) {
            adjacentindices.push_back(pair.second);
            adjacentindices.push_back(map[pair]);
        }
        else {
            std::cout << "no adjacency" << std::endl;
            adjacentindices.clear();
            return;
        }
    }
}
void SilhouetteDetection(const glm::vec4 lightPos ,const std::vector<glm::vec3>& incomingVerts,const std::vector<uint>& adjacency ,std::vector<uint>& edges);
void ExtrudeShadows(glm::mat4 projection ,const glm::vec4 lightPos ,const std::vector<glm::vec3>& incomingVerts,const std::vector<uint>& adjacency ,std::vector<glm::vec4>& emitedVerts);
void MakeShadow(float* verts , uint vSize , uint* indices , uint indSize, float*& outVert , uint*& outInd , uint16_t& outVSize, uint16_t& outIsize ) {
    glm::vec3 p;
    std::vector<glm::vec3> outV;
    for(int i = 0; i < vSize / 8 ; i++) {

        p.x = verts[i * 8 + 0];
        p.y = verts[i * 8 + 1];
        p.z = verts[i * 8 + 2];

        bool s = false;
        for(int k = 0; k < outV.size() ; k++ ) {
            if(p == outV[k] ) {
                s = true;
                break;
            }
        }

        if(!s) {
            outV.push_back(p);
        }
    }

    std::vector<uint> indexVector;
    for(int i = 0; i < indSize ;i++) {
        p.x = verts[indices[i] * 8 + 0];
        p.y = verts[indices[i] * 8 + 1];
        p.z = verts[indices[i] * 8 + 2];

        for(int k = 0; k < outV.size() ; k++) {
            if(p == outV[k]) {
                indexVector.push_back(k);
                break;
            }
        }
    }
    auto s = indexVector.size();

    std::vector<uint> outI;
    FindAdjecencies(indexVector , outI );

    if(outI.size() == 0) {
        outVert = nullptr;
        outInd = nullptr;
        outVSize = 0;
        outIsize = 0;
        return;
    }

    
    // std::vector<uint> edges;
    // edges.reserve(12000);
    // auto timer = std::chrono::high_resolution_clock::now();
    // SilhouetteDetection(glm::vec4(10,5,10,20) , outV , outI , edges);
    // auto end = std::chrono::high_resolution_clock::now();
    // auto deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(end-timer).count();
    // auto size = edges.size();
    // std::cout << deltaTime << " extrusion time " << size << std::endl;
    // std::terminate();


    outVert = (float*)new glm::vec3[outV.size()];
    outInd = new uint[outI.size()];

    for(int i = 0; i < outV.size() ; i++) {
        outVert[i * 3 + 0] = outV[i].x;
        outVert[i * 3 + 1] = outV[i].y;
        outVert[i * 3 + 2] = outV[i].z;
    }
    for(int i = 0; i < outI.size() ; i++) {
        outInd[i] = outI[i];
    }
    outVSize = outV.size() * sizeof(glm::vec3);
    outIsize = outI.size() * sizeof(uint);
}

bool operator == (const Mesh& p1 , const Mesh& p2) {
    return (p1.indOffset == p2.indOffset) & (p1.indSize == p2.indSize) & (p1.vertOffset == p2.vertOffset) & (p1.vertSize == p2.vertSize)
    & (p1.indOffsetS == p2.indOffsetS) & (p1.indSizeS == p2.indSizeS) & (p1.vertOffsetS == p2.vertOffsetS) & (p1.vertSizeS == p2.vertSizeS);
}

glm::mat3x3 MakeTransform(const Renderable& r) {
    return glm::eulerAngleXYZ(r.rot.x , r.rot.y , r.rot.z) * glm::scale(glm::mat4(1) , r.scale);
}

uint32_t Pad(uint32_t objSize , uint32_t padTo) {
    return ( (objSize / padTo) * padTo) + (((objSize % padTo) > 0) * padTo) - objSize;
}

void ColQuad( PackedAAQuad p ) {
    std::cout << (int)p.col.r << " " << (int)p.col.g << " " << (int)p.col.b << " " << (int)p.col.a << std::endl;
}
Mesh Helper(void * r , std::string file) {

    objl::Loader loader;
    if(!loader.LoadFile(file)) {
        std::terminate();
    }

    struct Vertex {
        glm::vec3 pos;
        glm::vec2 uv;
    };

    Vertex v[loader.LoadedVertices.size()];
    for(int i = 0; i < loader.LoadedVertices.size() ; i++ ) {
        v[i].pos.x = loader.LoadedVertices[i].Position.X;
        v[i].pos.y = loader.LoadedVertices[i].Position.Y;
        v[i].pos.z = loader.LoadedVertices[i].Position.Z;

        v[i].uv.x = loader.LoadedVertices[i].TextureCoordinate.X;
        v[i].uv.y = loader.LoadedVertices[i].TextureCoordinate.Y;
    }
    return RegisterMesh( r , v , loader.LoadedVertices.size() * sizeof(Vertex) , &loader.LoadedIndices[0] , loader.LoadedIndices.size() * 4 , true);
}


void MakeAAQuad(float*& write, const glm::vec4 quad) {
    write[0] = quad.x;
    write[1] = quad.w;
    
    // write[2] = 0;
    // write[3] = 0;

    write[4] = quad.z;
    write[5] = quad.w;

    // write[6] = 0;
    // write[7] = 0;

    write[8] = quad.z;
    write[9] = quad.y;

    // write[10] = 0;
    // write[11] = 0;

    write[12] = quad.x;
    write[13] = quad.y;

    // write[14] = 0;
    // write[15] = 0;

    write = write + 16;
}
void MakeUniformlyColoredAAQuad(void*& write, const PackedAAQuad& quad) {
    struct Vertex {
        glm::vec2 vPos;
        glm::vec<4,uint8_t> vCol;
    };

    ((Vertex*)write)[0].vPos.x = quad.bound.x;
    ((Vertex*)write)[0].vPos.y = quad.bound.w;
    ((Vertex*)write)[0].vCol = quad.col;

    ((Vertex*)write)[1].vPos.x = quad.bound.z;
    ((Vertex*)write)[1].vPos.y = quad.bound.w;
    ((Vertex*)write)[1].vCol = quad.col;

    ((Vertex*)write)[2].vPos.x = quad.bound.z;
    ((Vertex*)write)[2].vPos.y = quad.bound.y;
    ((Vertex*)write)[2].vCol = quad.col;

    ((Vertex*)write)[3].vPos.x = quad.bound.x;
    ((Vertex*)write)[3].vPos.y = quad.bound.y;
    ((Vertex*)write)[3].vCol = quad.col;

    write = ((Vertex*)write) + 4;
}
void MakeTexturedAAQuad(void*& write , const PackedTintedTexturedAAQuad& quad , const uint32_t index) {
    struct Vertex {
        glm::vec2 vPos;
        glm::vec<4,uint8_t> vCol;
        uint32_t texture;
    };

    ((Vertex*)write)[0].vPos.x = quad.bound.x;
    ((Vertex*)write)[0].vPos.y = quad.bound.w;
    ((Vertex*)write)[0].vCol = quad.col[0];
    ((Vertex*)write)[0].texture = index;

    ((Vertex*)write)[1].vPos.x = quad.bound.z;
    ((Vertex*)write)[1].vPos.y = quad.bound.w;
    ((Vertex*)write)[1].vCol = quad.col[1];
    ((Vertex*)write)[1].texture = index;

    ((Vertex*)write)[2].vPos.x = quad.bound.z;
    ((Vertex*)write)[2].vPos.y = quad.bound.y;
    ((Vertex*)write)[2].vCol = quad.col[2];
    ((Vertex*)write)[2].texture = index;

    ((Vertex*)write)[3].vPos.x = quad.bound.x;
    ((Vertex*)write)[3].vPos.y = quad.bound.y;
    ((Vertex*)write)[3].vCol = quad.col[3];
    ((Vertex*)write)[3].texture = index;

    write = ((Vertex*)write) + 4;
}
void MakeTextQuads( TextBox& text ,  Quad*& write) {
    
    const std::string& string = text.text;
    const Font& font = text.fontPtr->GetFont();
    glm::vec4 cursorLineScale = text.bounds;

    float vertSize = cursorLineScale.y - cursorLineScale.w;
    float horizSize = cursorLineScale.z - cursorLineScale.x;

    int writeIndex = 0;
    float unScaledTextWidth = 0;
    
    double pixelSizeX = (double)1 / (double)font.meta.scaleW;
    double pixelSizeY = (double)1 / (double)font.meta.scaleH;

    for(int i = 0; i < string.size() ;i++) {
        unScaledTextWidth += pixelSizeX * font.charMetaSetFast[string[i]].xAdvance;
    }
    
    float xScale = horizSize / unScaledTextWidth;
    float yScale = vertSize / (font.meta.maxHeight * pixelSizeY * 1.32); // idk
    pixelSizeX *= xScale;
    pixelSizeY *= yScale;
    cursorLineScale.x -= pixelSizeX * font.charMetaSetFast[string.front()].xOffset;

    for(int i = 0; i < string.size() ; i++ ) {

        float xCoord = cursorLineScale.x + pixelSizeX * font.charMetaSetFast[string[i]].xOffset;
        float yCoord = cursorLineScale.y - pixelSizeY * font.charMetaSetFast[string[i]].yOffset;
        cursorLineScale.x += pixelSizeX * font.charMetaSetFast[string[i]].xAdvance;

        if( string[i] != ' ' ) {
            write[writeIndex] = font.charQuadSet[string[i]] * glm::vec4(xCoord , yCoord , xScale , yScale);
            writeIndex++;
        }
    }

    write += writeIndex;
}
void MakeTextQuads2( Text3D& text ,  Quad*& write) {

    const std::string& string = text.text;
    const Font& font = text.fontPtr->GetFont();
    glm::vec4 cursorLineScale = text.bounds;

    float vertSize = cursorLineScale.y - cursorLineScale.w;
    float horizSize = cursorLineScale.z - cursorLineScale.x;

    int writeIndex = 0;
    float unScaledTextWidth = 0;
    
    double pixelSizeX = (double)1 / (double)font.meta.scaleW;
    double pixelSizeY = (double)1 / (double)font.meta.scaleH;

    for(int i = 0; i < string.size() ;i++) {
        unScaledTextWidth += pixelSizeX * font.charMetaSet[string[i]].xAdvance;
    }
    
    float xScale = horizSize / unScaledTextWidth;
    float yScale = vertSize / (font.meta.maxHeight * pixelSizeY * 1.32); // idk
    pixelSizeX *= xScale;
    pixelSizeY *= yScale;
    cursorLineScale.x -= pixelSizeX * font.charMetaSet[string.front()].xOffset;

    for(int i = 0; i < string.size() ; i++ ) {

        float xCoord = cursorLineScale.x + pixelSizeX * font.charMetaSet[string[i]].xOffset;
        float yCoord = cursorLineScale.y - pixelSizeY * font.charMetaSet[string[i]].yOffset;
        cursorLineScale.x += pixelSizeX * font.charMetaSet[string[i]].xAdvance;

        if( string[i] != ' ' ) {
            write[writeIndex] = font.charQuadSet[string[i]] * glm::vec4(xCoord , yCoord , xScale , yScale);
            writeIndex++;
        }
    }

    write += writeIndex;
}

unsigned int Quad::indecies[6] = {0,1,2,2,3,0};
Quad Quad::operator * ( glm::vec4 transform ) const {

    Quad ret = *this;
    for(int i = 0; i < 4 ; i++ ) {
        ret.vertecies[i].vertexPosition.x *= transform.z;
        ret.vertecies[i].vertexPosition.y *= transform.w;
    }

    float sizeX = ret.vertecies[1].vertexPosition.x;
    float sizeY = ret.vertecies[0].vertexPosition.y;

    ret.vertecies[0].vertexPosition.x = transform.x;
    ret.vertecies[0].vertexPosition.y = transform.y;

    ret.vertecies[1].vertexPosition.x = ret.vertecies[0].vertexPosition.x + sizeX;
    ret.vertecies[1].vertexPosition.y = ret.vertecies[0].vertexPosition.y;
    
    ret.vertecies[2].vertexPosition.x = ret.vertecies[1].vertexPosition.x;
    ret.vertecies[2].vertexPosition.y = ret.vertecies[1].vertexPosition.y - sizeY;
    
    ret.vertecies[3].vertexPosition.x = ret.vertecies[0].vertexPosition.x;
    ret.vertecies[3].vertexPosition.y = ret.vertecies[0].vertexPosition.y - sizeY;

    return ret;
}

void SilhouetteDetection(const glm::vec4 lightPos ,const std::vector<glm::vec3>& incomingVerts,const std::vector<uint>& adjacency ,std::vector<uint>& edges) {

    glm::vec3 worldPos[6];
    glm::vec3 e[6];
    for(int i = 0; i < adjacency.size() ; i+= 6) {

        worldPos[0] = incomingVerts[adjacency[i + 0]];
        worldPos[2] = incomingVerts[adjacency[i + 2]];
        worldPos[4] = incomingVerts[adjacency[i + 4]];

        e[0] = worldPos[2] - worldPos[0];
        e[1] = worldPos[4] - worldPos[0];

        glm::vec3 normal = glm::cross(e[0],e[1]);
        glm::vec3 lightDir = glm::vec3(lightPos) - worldPos[0];

        if( glm::dot(normal,lightDir) > 0 ) {

            worldPos[1] = incomingVerts[adjacency[i + 1]];
            worldPos[3] = incomingVerts[adjacency[i + 3]];
            worldPos[5] = incomingVerts[adjacency[i + 5]];
            e[2] = worldPos[1] - worldPos[0];
            e[3] = worldPos[3] - worldPos[2];
            e[4] = worldPos[4] - worldPos[2];
            e[5] = worldPos[5] - worldPos[0];


            normal = glm::cross(e[2],e[0]);
            if( glm::dot(normal,lightDir) > 0 ) {
                edges.push_back(adjacency[i + 0]);
                edges.push_back(adjacency[i + 2]);
            }

            normal = glm::cross(e[3],e[4]);
            lightDir = glm::vec3(lightPos) - worldPos[2];

            if( glm::dot(normal,lightDir) > 0 ) {
                edges.push_back(adjacency[i + 0]);
                edges.push_back(adjacency[i + 4]);
            }

            normal = glm::cross(e[1],e[5]);
            lightDir = glm::vec3(lightPos) - worldPos[4];
            if( glm::dot(normal,lightDir) > 0 ) {
                edges.push_back(adjacency[i + 4]);
                edges.push_back(adjacency[i + 2]);
            }
        }
    }
}

void ExtrudeShadows(glm::mat4 projection ,const glm::vec4 lightPos ,const std::vector<glm::vec3>& incomingVerts,const std::vector<uint>& adjacency ,std::vector<glm::vec4>& emitedVerts) {

    glm::vec3 worldPos[6];
    glm::vec3 e[6];
    for(int i = 0; i < adjacency.size() ; i+= 6) {

        worldPos[0] = incomingVerts[adjacency[i + 0]];
        worldPos[2] = incomingVerts[adjacency[i + 2]];
        worldPos[4] = incomingVerts[adjacency[i + 4]];

        e[0] = worldPos[2] - worldPos[0];
        e[1] = worldPos[4] - worldPos[0];

        glm::vec3 normal = glm::cross(e[0],e[1]);
        glm::vec3 lightDir = glm::vec3(lightPos) - worldPos[0];

        if( glm::dot(normal,lightDir) > 0 ) {

            worldPos[1] = incomingVerts[adjacency[i + 1]];
            worldPos[3] = incomingVerts[adjacency[i + 3]];
            worldPos[5] = incomingVerts[adjacency[i + 5]];
            e[2] = worldPos[1] - worldPos[0];
            e[3] = worldPos[3] - worldPos[2];
            e[4] = worldPos[4] - worldPos[2];
            e[5] = worldPos[5] - worldPos[0];


            normal = glm::cross(e[2],e[0]);
            if( glm::dot(normal,lightDir) > 0 ) {

                glm::vec4 triangle_strip[6];

                lightDir = glm::normalize(worldPos[0] - glm::vec3(lightPos));
                triangle_strip[0] = projection * glm::vec4( worldPos[0] + lightDir * 0.01f , 1 );

                triangle_strip[1] = projection * glm::vec4(glm::vec3(lightPos) + lightDir * lightPos.w , 1);

                lightDir = glm::normalize(worldPos[2] - glm::vec3(lightPos));
                triangle_strip[2] = projection * glm::vec4( worldPos[2] + lightDir * 0.01f , 1 );

                triangle_strip[3] = triangle_strip[2];
                triangle_strip[5] = triangle_strip[1];
                triangle_strip[4] = projection * glm::vec4(glm::vec3(lightPos) + lightDir * lightPos.w , 1);

                emitedVerts.push_back(triangle_strip[0]);
                emitedVerts.push_back(triangle_strip[1]);
                emitedVerts.push_back(triangle_strip[2]);

                emitedVerts.push_back(triangle_strip[3]);
                emitedVerts.push_back(triangle_strip[4]);
                emitedVerts.push_back(triangle_strip[5]);
            }
            
            normal = glm::cross(e[3],e[4]);
            lightDir = glm::vec3(lightPos) - worldPos[2];

            if( glm::dot(normal,lightDir) > 0 ) {

                glm::vec4 triangle_strip[6];

                lightDir = glm::normalize(worldPos[0] - glm::vec3(lightPos));
                triangle_strip[0] = projection * glm::vec4( worldPos[0] + lightDir * 0.01f , 1 );

                triangle_strip[1] = projection * glm::vec4(glm::vec3(lightPos) + lightDir * lightPos.w , 1);

                lightDir = glm::normalize(worldPos[4] - glm::vec3(lightPos));
                triangle_strip[2] = projection * glm::vec4( worldPos[4] + lightDir * 0.01f , 1 );

                triangle_strip[3] = triangle_strip[2];
                triangle_strip[5] = triangle_strip[1];
                triangle_strip[4] = projection * glm::vec4(glm::vec3(lightPos) + lightDir * lightPos.w , 1);
            }

            normal = glm::cross(e[1],e[5]);
            lightDir = glm::vec3(lightPos) - worldPos[4];

            if( glm::dot(normal,lightDir) > 0 ) {

                glm::vec4 triangle_strip[6];

                lightDir = glm::normalize(worldPos[4] - glm::vec3(lightPos));
                triangle_strip[0] = projection * glm::vec4( worldPos[4] + lightDir * 0.01f , 1 );

                triangle_strip[1] = projection * glm::vec4(glm::vec3(lightPos) + lightDir * lightPos.w , 1);

                lightDir = glm::normalize(worldPos[2] - glm::vec3(lightPos));
                triangle_strip[2] = projection * glm::vec4( worldPos[2] + lightDir * 0.01f , 1 );

                triangle_strip[3] = triangle_strip[2];
                triangle_strip[5] = triangle_strip[1];
                triangle_strip[4] = projection * glm::vec4(glm::vec3(lightPos) + lightDir * lightPos.w , 1);

                emitedVerts.push_back(triangle_strip[0]);
                emitedVerts.push_back(triangle_strip[1]);
                emitedVerts.push_back(triangle_strip[2]);
                
                emitedVerts.push_back(triangle_strip[3]);
                emitedVerts.push_back(triangle_strip[4]);
                emitedVerts.push_back(triangle_strip[5]);
            }

            glm::vec3 lightDirs[3];
            lightDirs[0] = glm::normalize(worldPos[0] - glm::vec3(lightPos));
            glm::vec4 emit = projection * glm::vec4( glm::vec3(lightPos) + lightDirs[0] * lightPos.w ,1);
            emitedVerts.push_back(emit);

            lightDirs[1] = glm::normalize(worldPos[4] - glm::vec3(lightPos));
            emit = projection * glm::vec4( glm::vec3(lightPos) + lightDirs[1] * lightPos.w ,1);
            emitedVerts.push_back(emit);
            
            lightDirs[2] = glm::normalize(worldPos[2] - glm::vec3(lightPos));
            emit = projection * glm::vec4( glm::vec3(lightPos) + lightDirs[2] * lightPos.w ,1);
            emitedVerts.push_back(emit);


            emit = projection * glm::vec4( worldPos[0] + lightDirs[0] * 0.01 , 1);
            emitedVerts.push_back(emit);
            emit = projection * glm::vec4( worldPos[2] + lightDirs[2] * 0.01 , 1);
            emitedVerts.push_back(emit);
            emit = projection * glm::vec4( worldPos[4] + lightDirs[1] * 0.01 , 1);
            emitedVerts.push_back(emit);
        }
    }
}