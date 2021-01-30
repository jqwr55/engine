#include <Types.h>

void FindAdjecencies(const std::vector<unsigned int>& indices , std::vector<unsigned int>& adjacentindices);
void MakeShadow(float* verts , uint vSize , uint* indices , uint indSize, float*& outVert , uint*& outInd , uint16_t& outVSize, uint16_t& outIsize );
bool operator == (const Mesh& p1 , const Mesh& p2);
glm::mat3x3 MakeTransform(const Renderable& r);
uint32_t Pad(uint32_t objSize , uint32_t padTo);
void ColQuad( PackedAAQuad p );
Mesh Helper(void * r , std::string file);
void DrawLine(glm::vec2 p0 , glm::vec2 p1 , glm::vec3 color);
glm::vec4 DrawEllispeBoundingBox(glm::vec2 c , glm::vec2 r , float angle);
glm::mat4 PerspectiveMatrix(float fov , float aspect , float near , float far);
glm::mat4 LookAt(glm::vec3 from , glm::vec3 to , glm::vec3 worldUp = {0,1,0} );
void GetBoundsForAxis(glm::vec3 axis , glm::vec3 C , float r , float nearZ , glm::vec3& L , glm::vec3& U);
void GetLightAABB(glm::vec4 sphere , CommonShaderParams& params , glm::vec4& bounds);
void SetLightScissor(glm::vec4 sphere , CommonShaderParams& params , Shader& debugShader);
void MakeAAQuad(float*& write, const glm::vec4 quad);
void MakeUniformlyColoredAAQuad(void*& write, const PackedAAQuad& quad);
void MakeTexturedAAQuad(void*& write , const PackedTintedTexturedAAQuad& quad , const uint32_t index);
void MakeTextQuads( TextBox& text ,  Quad*& write);
void MakeTextQuads2( Text3D& text ,  Quad*& write);
void extract_planes_from_projmat(const float mat[4][4], float left[4], float right[4], float top[4], float bottom[4], float near[4], float far[4]);
float DistanceToPlane( glm::vec4 vPlane, glm::vec3 vPoint );

// Frustum cullling on a sphere. Returns > 0 if visible, <= 0 otherwise
float CullSphere( glm::vec4 vPlanes[6], glm::vec3 vCenter, float fRadius );

bool Any_4x32(glm::vec<4,uint32_t> c);
bool All_4x32(glm::vec<4,uint32_t> c);
glm::vec<4,uint32_t> LessThan(glm::vec4 a , glm::vec4 b);
glm::vec4 exp_4x32(glm::vec4 x);
glm::vec4 tan_4x32(glm::vec4 x);
glm::vec4 F_4x32(glm::vec4 u , glm::vec4 v, float steps);
float F(float u , float v, int steps) ;
void MakeFuncLookupTable(float* tex , int size , float steps);
void ExtrudeShadows(glm::mat4 projection ,const glm::vec4 lightPos ,const std::vector<glm::vec3>& incomingVerts,const std::vector<uint>& adjacency ,std::vector<glm::vec4>& emitedVerts);
