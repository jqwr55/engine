#shader:vertex
#version 460 core

layout(location = 0) in vec3 vertexPos;
layout(location = 1) in vec2 vertexUV;

layout(location = 2) in vec3 row0;
layout(location = 3) in vec3 row1;
layout(location = 4) in vec3 row2;
layout(location = 5) in vec3 row3;
layout(location = 6) in uint textureIndex;


layout(std140 , binding = 1) uniform CommonRenderParams {
    mat4 projectionViewMatrix;
    mat4 inverseProjectionViewMatrix;
    vec4 viewDir;
    vec4 viewPos;
    vec4 viewRight;
    vec4 viewUp;
};

out vec3 fragCol;
out vec2 fragUV;
flat out uint fragTextureIndex;

void main() {
    fragUV = vertexUV;
    fragTextureIndex = textureIndex;
    gl_Position = projectionViewMatrix * vec4((mat3(row0,row1,row2) * vertexPos) + row3 , 1);
}

#shader:fragment
#version 460 core
#extension GL_ARB_bindless_texture : enable
#extension GL_NV_gpu_shader5 : enable
#extension GL_ARB_gpu_shader_int64 : enable

layout(location = 0) out vec4 outCol;

layout(binding = 2) uniform TextureAddresses {
    uint64_t texAddresses[500];
};

flat in uint fragTextureIndex;
in vec3 fragCol;
in vec2 fragUV;

void main() {
    sampler2D textureSampler = sampler2D(texAddresses[fragTextureIndex]);
    vec4 c = texture(textureSampler , fragUV);
    outCol = c;
}