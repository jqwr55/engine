#shader:vertex
#version 460 core

layout(location = 0) in vec3 vertexPos;
layout(location = 1) in vec3 vertexCol;

layout(location = 2) in vec3 row0;
layout(location = 3) in vec3 row1;
layout(location = 4) in vec3 row2;
layout(location = 5) in vec3 row3;

layout(std140 , binding = 1) uniform CommonRenderParams {
    mat4 projectionViewMatrix;
    mat4 inverseProjectionViewMatrix;
    vec4 viewDir;
    vec4 viewPos;
    vec4 viewRight;
    vec4 viewUp;
};

layout(location = 1) uniform vec4 gLightPos;
const float EPSILON = 0.0001f;
out vec3 fragCol;

subroutine void VertexTranformFunction();
layout(location = 0) subroutine uniform VertexTranformFunction VertexTranformFunctionUniform;
layout(index = 0) subroutine(VertexTranformFunction) void FrontCap() {
    fragCol = vertexCol;

    vec4 p = vec4(mat3(row0,row1,row2) * vertexPos + row3,1);
    vec3 lightDir = normalize(p.xyz - gLightPos.xyz);

    p.xyz += lightDir * EPSILON;
    gl_Position = projectionViewMatrix * p;
}
layout(index = 1) subroutine(VertexTranformFunction) void BackCap() {
    fragCol = vertexCol;

    vec4 p = vec4(mat3(row0,row1,row2) * vertexPos + row3,1);
    vec3 lightDir = normalize(p.xyz - gLightPos.xyz);

    p.xyz = gLightPos.xyz + lightDir * gLightPos.w;
    gl_Position = projectionViewMatrix * p;
}


void main() {
    VertexTranformFunctionUniform();
}

#shader:fragment
#version 460 core

layout(location = 0) out vec4 outCol;

in vec3 fragCol;

void main() {
    outCol = vec4(1);
}