#shader:vertex
#version 460 core

layout(location = 0) in vec3 vertexPos;
layout(location = 1) in vec3 vertexCol;

layout(std140 , binding = 1) uniform CommonRenderParams {
    mat4 projectionViewMatrix;
    mat4 inverseProjectionViewMatrix;
    vec4 viewDir;
    vec4 viewPos;
    vec4 viewRight;
    vec4 viewUp;
};

out vec3 fragCol;
void main() {
    fragCol = vertexCol;
    gl_Position = projectionViewMatrix * vec4(vertexPos,1);
}

#shader:fragment
#version 460 core

layout(location = 0) out vec4 outCol;

in vec3 fragCol;

void main() {
    outCol = vec4(fragCol , 1);
}