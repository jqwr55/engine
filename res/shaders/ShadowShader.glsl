#shader:vertex
#version 460 core

layout(location = 0) in vec3 vertexPos;

layout(location = 1) in vec3 row1;
layout(location = 2) in vec3 row2;
layout(location = 3) in vec3 row3;
layout(location = 4) in vec3 row4;

flat out vec3 worldPos;

void main() {
    worldPos = (mat3x3(row1,row2,row3) * vertexPos) + row4;
}

#shader:geometry
#version 460 core

layout(triangles_adjacency) in;
layout(triangle_strip, max_vertices = 18) out;

layout(location = 1) uniform vec4 gLightPos;

layout(std140 , binding = 0) uniform CommonRenderParams {
    mat4 projectionViewMatrix;
    mat4 inverseProjectionViewMatrix;
    vec4 viewUp;
    vec4 viewRight;
    vec4 viewDir;
    vec4 viewPos;
	uvec4 resolutionInverse;
    float Time;
};

const float EPSILON = 0.01f;

in vec3 worldPos[];
//out vec4 shadowVertex;

void EmitQuad(vec3 StartVertex, vec3 EndVertex) {
    vec3 LightDir = normalize(StartVertex - gLightPos.xyz);
    vec4 outPut = projectionViewMatrix * vec4((StartVertex + LightDir * EPSILON), 1.0);
    gl_Position = outPut;
    //shadowVertex = outPut;
    EmitVertex();

    outPut = projectionViewMatrix * vec4(gLightPos.xyz + LightDir * gLightPos.w, 1.0);
    gl_Position = outPut;
    //shadowVertex = outPut;
    EmitVertex();

    LightDir = normalize(EndVertex - gLightPos.xyz);
    outPut = projectionViewMatrix * vec4((EndVertex + LightDir * EPSILON), 1.0);
    gl_Position = outPut;
    //shadowVertex = outPut;
    EmitVertex();

    outPut = projectionViewMatrix * vec4(gLightPos.xyz + LightDir * gLightPos.w , 1.0);
    gl_Position = outPut;
    //shadowVertex = outPut;
    EmitVertex();

    EndPrimitive();
}

void main() {
    vec3 e0 = worldPos[2] - worldPos[0];
    vec3 e1 = worldPos[4] - worldPos[0];
    vec3 e2 = worldPos[1] - worldPos[0];

    vec3 e3 = worldPos[3] - worldPos[2];
    vec3 e4 = worldPos[4] - worldPos[2];
    vec3 e5 = worldPos[5] - worldPos[0];

    vec3 Normal = cross(e0,e1);
    vec3 LightDir = gLightPos.xyz - worldPos[0];

    if(dot(Normal , LightDir) > 0) {
        
        Normal = cross(e2,e0);

        if(dot(Normal , LightDir) <= 0) {
            vec3 StartVertex = worldPos[0];
            vec3 EndVertex = worldPos[2];
            EmitQuad(StartVertex , EndVertex);
        }
        Normal = cross(e3 ,e4);
        LightDir = gLightPos.xyz - worldPos[2];

        if(dot(Normal , LightDir) <= 0) {
            vec3 StartVertex = worldPos[2];
            vec3 EndVertex = worldPos[4];
            EmitQuad(StartVertex, EndVertex);
        }

        Normal = cross(e1 ,e5);
        LightDir = gLightPos.xyz - worldPos[4];

        if(dot(Normal , LightDir) <= 0) {
            vec3 StartVertex = worldPos[4];
            vec3 EndVertex = worldPos[0];
            EmitQuad(StartVertex, EndVertex);
        }

        vec3 lightDirs[3];

        lightDirs[0] = normalize(worldPos[0] - gLightPos.xyz);
        vec4 outPut = projectionViewMatrix * vec4(gLightPos.xyz + lightDirs[0] * gLightPos.w, 1.0);
        gl_Position = outPut;
        //shadowVertex = outPut;
        EmitVertex();

        lightDirs[1] = normalize(worldPos[4] - gLightPos.xyz);
        outPut = projectionViewMatrix * vec4(gLightPos.xyz + lightDirs[1] * gLightPos.w, 1.0);
        gl_Position = outPut;
        //shadowVertex = outPut;
        EmitVertex();

        lightDirs[2] = normalize(worldPos[2] - gLightPos.xyz);
        outPut = projectionViewMatrix * vec4(gLightPos.xyz + lightDirs[2] * gLightPos.w, 1.0);
        gl_Position = outPut;
        //shadowVertex = outPut;
        EmitVertex();
        EndPrimitive();
        
        outPut = projectionViewMatrix * vec4((worldPos[0] + lightDirs[0] * EPSILON), 1.0);
        gl_Position = outPut;
        //shadowVertex = outPut;
        EmitVertex();

        outPut = projectionViewMatrix * vec4((worldPos[2] + lightDirs[2] * EPSILON), 1.0);
        gl_Position = outPut;
        //shadowVertex = outPut;
        EmitVertex();

        outPut = projectionViewMatrix * vec4((worldPos[4] + lightDirs[1] * EPSILON), 1.0);
        gl_Position = outPut;
        //shadowVertex = outPut;
        EmitVertex();
        EndPrimitive();
    }
}


void FrontAndBackCapProjection_OLD() {

        vec3 LightDir = worldPos[0] - gLightPos.xyz;
        vec4 outPut = projectionViewMatrix * vec4(LightDir, 0.0);
        gl_Position = outPut;
        //shadowVertex = outPut;
        EmitVertex();

        LightDir = worldPos[4] - gLightPos.xyz;
        outPut = projectionViewMatrix * vec4(LightDir, 0.0);
        gl_Position = outPut;
        //shadowVertex = outPut;
        EmitVertex();

        LightDir = worldPos[2] - gLightPos.xyz;
        outPut = projectionViewMatrix * vec4(LightDir, 0.0);
        gl_Position = outPut;
        //shadowVertex = outPut;
        EmitVertex();
        EndPrimitive();
        
        LightDir = (normalize(worldPos[0] - gLightPos.xyz));
        outPut = projectionViewMatrix * vec4((worldPos[0] + LightDir * EPSILON), 1.0);
        gl_Position = outPut;
        //shadowVertex = outPut;
        EmitVertex();

        LightDir = (normalize(worldPos[2] - gLightPos.xyz));
        outPut = projectionViewMatrix * vec4((worldPos[2] + LightDir * EPSILON), 1.0);
        gl_Position = outPut;
        //shadowVertex = outPut;
        EmitVertex();

        LightDir = (normalize(worldPos[4] - gLightPos.xyz));
        outPut = projectionViewMatrix * vec4((worldPos[4] + LightDir * EPSILON), 1.0);
        gl_Position = outPut;
        //shadowVertex = outPut;
        EmitVertex();
        EndPrimitive();
}