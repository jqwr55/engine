#pragma once

#include <vector>

#include <Debug.h>
#include <BufferObject.h>

struct VertexAttributeProperties {
    GLenum dataType;
    GLenum shaderType;
    uint8_t index;
    uint8_t size;
    uint8_t attribDivisor;
    bool normalized;
};

class VertexArrayLayoutDescriptor {
    public:
        uint16_t stride = 0;
        //std::vector<VertexAttributeProperties> elements;
        static std::vector<VertexAttributeProperties> pool;
        uint16_t offset;
        uint8_t size;

        VertexArrayLayoutDescriptor();
        ~VertexArrayLayoutDescriptor();
        static const unsigned int GetSizeOf( const unsigned int type);
        void Push(const GLenum dataType ,const GLenum shaderType ,const uint16_t Count ,const uint16_t Attribdivisor = 0);
        void Begin();
};

class VertexArray {
    public:
        VertexArray();
        VertexArray(const VertexArray& other) = delete;
        VertexArray(VertexArray&& temp);

        VertexArray& operator = (const VertexArray& other) = delete;
        VertexArray& operator = (VertexArray&& temp);
        ~VertexArray();
        //void DeleteID();
        //void NewID();
        void AddBuffer(const BufferObject<GL_ARRAY_BUFFER>& vbo , const VertexArrayLayoutDescriptor& layout);
        void AddBuffer(const BufferObject<GL_ELEMENT_ARRAY_BUFFER>& vbo );
        void NullIndex();
        void Bind();
        void UnBind();
        const unsigned int& GetArrayID();
    private:
        uint32_t vertexArrayID;
        uint32_t index;
};