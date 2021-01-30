#include <VertexArray.h>
#include <iostream>

std::vector<VertexAttributeProperties> VertexArrayLayoutDescriptor::pool{};

VertexArrayLayoutDescriptor::VertexArrayLayoutDescriptor() {
    stride = 0;
}
VertexArrayLayoutDescriptor::~VertexArrayLayoutDescriptor() {
    //pool.erase( pool.begin() + offset , pool.begin() + offset + size );
}

void VertexArrayLayoutDescriptor::Begin() {
    offset = pool.size();
    size = 0;
}
void VertexArrayLayoutDescriptor::Push(const GLenum dataType ,const GLenum shaderType ,const uint16_t Count ,const uint16_t Attribdivisor) {

    VertexAttributeProperties vbl;
    vbl.index = size;
    vbl.size = Count;
    vbl.dataType = dataType;
    vbl.shaderType = shaderType;
    vbl.attribDivisor = Attribdivisor;
    switch (dataType) {
        case GL_FLOAT:
            vbl.normalized = GL_FALSE;
            stride+= (GetSizeOf(GL_FLOAT) * Count);
            break;
        case GL_INT:
            vbl.normalized = GL_FALSE;
            stride+= (GetSizeOf(GL_INT) * Count);
            break;
        case GL_UNSIGNED_INT:
            vbl.normalized = GL_FALSE;
            stride+= (GetSizeOf(GL_UNSIGNED_INT) * Count);
            break;
    default:
            vbl.normalized = GL_FALSE;
            stride+= (4 * Count);
            break;
    }
    pool.insert(pool.begin() + offset + size, vbl);
    size++;

    //elements.push_back(vbl);
}

const unsigned int VertexArrayLayoutDescriptor::GetSizeOf(unsigned int type) {
    switch (type) {
        case GL_FLOAT:
            return 4;
        case GL_INT:
            return 4;
        case GL_UNSIGNED_INT:
            return 4;
        case GL_BYTE:
            return 1;
        default:
            return 4;
    }
}

VertexArray::VertexArray() {
    GLCall(glGenVertexArrays(1 , &vertexArrayID));
    index = 0;
}
VertexArray::VertexArray(VertexArray&& temp) {
    vertexArrayID = temp.vertexArrayID;
    index = temp.index;
    GLCall(glGenVertexArrays(1 , &temp.vertexArrayID));
}
VertexArray::~VertexArray() {
    GLCall(glDeleteVertexArrays(1,&vertexArrayID));
}

VertexArray& VertexArray::operator =(VertexArray&& temp) {
    vertexArrayID = temp.vertexArrayID;
    index = temp.index;
    GLCall(glGenVertexArrays(1 , &temp.vertexArrayID));
    return *this;
}

void VertexArray::AddBuffer(const BufferObject<GL_ARRAY_BUFFER>& vbo , const VertexArrayLayoutDescriptor& layout) {
    GLCall(glBindVertexArray(vertexArrayID));
    vbo.Bind();

    uint64_t pointer = 0;
    for(int i = layout.offset ; i < layout.offset + layout.size ; i++) {

        const VertexAttributeProperties& vbl = layout.pool[i];

        GLCall(glEnableVertexAttribArray(index));
        switch (vbl.shaderType) {
        case GL_FLOAT:
            GLCall(glVertexAttribPointer(index,vbl.size , vbl.dataType , vbl.normalized , layout.stride , (const void*)pointer));
            break;
        case GL_INT:
            GLCall(glVertexAttribIPointer(index,vbl.size , vbl.dataType , layout.stride , (const void*)pointer));
            break;
        case GL_DOUBLE:
            GLCall(glVertexAttribLPointer(index,vbl.size , vbl.dataType , layout.stride , (const void*)pointer));
            break;
        default:
            GLCall(glVertexAttribPointer(index,vbl.size , vbl.dataType , vbl.normalized , layout.stride , (const void*)pointer));
            break;
        }
        GLCall(glVertexAttribDivisor(index , vbl.attribDivisor));

        pointer += (vbl.size * VertexArrayLayoutDescriptor::GetSizeOf(vbl.dataType));
        index++;
    }
}

void VertexArray::AddBuffer( const BufferObject<GL_ELEMENT_ARRAY_BUFFER>& ibo ) {
    GLCall(glVertexArrayElementBuffer(vertexArrayID , ibo.GetID() ));
}
void VertexArray::NullIndex() {
    index = 0;
}

const unsigned int& VertexArray::GetArrayID() {
    return vertexArrayID;
}

void VertexArray::Bind() {
    GLCall(glBindVertexArray(vertexArrayID));
}

void VertexArray::UnBind() {
    GLCall(glBindVertexArray(0));
}