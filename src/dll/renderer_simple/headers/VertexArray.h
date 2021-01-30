#pragma once

#include <vector>

#include <Debug.h>
#include <BufferObject.h>
#include <types.h>

enum ShaderType {
    SHADER_TYPE_INT,
    SHADER_TYPE_FLOAT,
    SHADER_TYPE_DOUBLE,
};

struct VertexAttributeProperties {
    u32 dataType;
    u8 shaderType;
    u8 size;
    u8 attribDivisor;
    u8 normalized;
};

struct VertexLayoutDescriptor {
    VertexAttributeProperties* attributes;
    u8 i;

    void Begin( void* mem ) {
        attributes = (VertexAttributeProperties*)mem;
        i = 0;
    }
    void PushAttribute(u32 size , u32 dataType , u32 shaderType , bool normalized , u32 divisor) {
        attributes[i++] = { dataType , (u8)shaderType, (u8)size , (u8)divisor , (u8)normalized };
    }
    void End(void*& mem) {
        mem = (void*)(attributes + i);
    }
    void Free(void*& mem) {
        mem = (void*)attributes;
        attributes = nullptr;
        i = 0;
    }
};

const u32 SIZEOF_GL_TYPE(u32 type) {
    switch (type) {
        case GL_FLOAT:
            return 4;
        case GL_INT:
            return 4;
        case GL_UNSIGNED_INT:
            return 4;
        case GL_UNSIGNED_BYTE:
            return 4;
        case GL_BYTE:
            return 4;
        case GL_HALF_FLOAT:
            return 4;
        case GL_DOUBLE:
            return 4;
        case GL_SHORT:
            return 4;
        case GL_UNSIGNED_SHORT:
            return 4;

        default:
            return 4;
    }
}

struct VertexArray {
    u32 state;
    u32 gl_handle;
    u32 index;
};

void AttachVBO(VertexArray& vao , const VertexLayoutDescriptor& descritpor , BufferObject<GL_ARRAY_BUFFER>& buffer) {

    GLCall(glBindVertexArray(vao.gl_handle));
    buffer.Bind();

    void* pointer = nullptr;
    u32 stride = 0;
    for(u32 i = 0; i < descritpor.i; i++) {
        stride += descritpor.attributes[i].size * SIZEOF_GL_TYPE( descritpor.attributes[i].dataType );
    }

    for(u32 i = 0 ; i < descritpor.i; i++) {

        GLCall(glEnableVertexAttribArray(vao.index));
        switch ( descritpor.attributes[i].shaderType ) {
            case SHADER_TYPE_FLOAT:
                GLCall(glVertexAttribPointer(vao.index, descritpor.attributes[i].size,
                                                        descritpor.attributes[i].dataType,
                                                        descritpor.attributes[i].normalized,
                                                        stride, pointer));
                break;
            case SHADER_TYPE_INT:
                GLCall(glVertexAttribIPointer(vao.index,descritpor.attributes[i].size,
                                                        descritpor.attributes[i].dataType,
                                                        stride, pointer));
                break;
            case SHADER_TYPE_DOUBLE:
                GLCall(glVertexAttribLPointer(vao.index, descritpor.attributes[i].size,
                                                        descritpor.attributes[i].dataType,
                                                        stride, pointer));
                break;
            
            default:
                break;
        }
        GLCall(glVertexAttribDivisor(vao.index,descritpor.attributes[i].attribDivisor));
        pointer = (void*)((byte*)pointer + descritpor.attributes[i].size * SIZEOF_GL_TYPE( descritpor.attributes[i].dataType ));
        vao.index++;
    }
}

void AttachIBO(VertexArray& vao , BufferObject<GL_ELEMENT_ARRAY_BUFFER>& buffer) {
    GLCall(glVertexArrayElementBuffer(vao.gl_handle , buffer.gl_handle));
}