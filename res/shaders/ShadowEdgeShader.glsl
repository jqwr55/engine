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

out vec3 worldPos;

void main() {
    worldPos = (mat3(row0,row1,row2) * vertexPos) + row3;
}

#shader:geometry
#version 460 core

layout(lines) in;
layout(triangle_strip, max_vertices = 4) out;

layout(location = 1) uniform vec4 gLightPos;

layout(std140 , binding = 1) uniform CommonRenderParams {
    mat4 projectionViewMatrix;
    mat4 inverseProjectionViewMatrix;
    vec4 viewDir;
    vec4 viewPos;
    vec4 viewRight;
    vec4 viewUp;
};

const float EPSILON = 0.0001f;

in vec3 worldPos[];

void main() {

    vec3 lightDir[2];

    lightDir[0] = normalize(worldPos[0] - gLightPos.xyz);
    gl_Position = projectionViewMatrix * vec4( fma(lightDir[0],EPSILON.xxx ,worldPos[0]) , 1.0f );
    EmitVertex();
    
    gl_Position = projectionViewMatrix * vec4( fma(lightDir[0] , gLightPos.www , gLightPos.xyz ) , 1.0f);
    EmitVertex();

    lightDir[1] = normalize(worldPos[1] - gLightPos.xyz);
    gl_Position = projectionViewMatrix * vec4( fma(lightDir[1],EPSILON.xxx,worldPos[1]), 1.0f );
    EmitVertex();

    gl_Position = projectionViewMatrix * vec4( fma(lightDir[1] , gLightPos.www , gLightPos.xyz ) , 1.0f);
    EmitVertex();
}

#shader:fragment
#version 460 core

layout(location = 0) out vec4 outCol;

void main() {
    outCol = vec4(1,0,0,1);
}