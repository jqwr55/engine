#pragma once

#include <chrono>
#include <iostream>
#include <Debug.h>
#include <Shader.h>

template<const uint32_t n > const char* GetEnum() {
    switch( n ) {
        case GL_ARRAY_BUFFER :
        return "GL_ARRAY_BUFFER";

        case GL_ELEMENT_ARRAY_BUFFER :
        return "GL_ELEMENT_ARRAY_BUFFER";
        
        case GL_UNIFORM_BUFFER :
        return "GL_UNIFORM_BUFFER";

        case GL_DRAW_INDIRECT_BUFFER :
        return "GL_DRAW_INDIRECT_BUFFER";

        default:
        return "UNKNOWN";
    }
}

template<const uint32_t type , const uint32_t usage = GL_DYNAMIC_DRAW> class BufferObject {
    public:
        BufferObject() {
            GLCall(glGenBuffers( 1 , &m_bufferID ));
            GLCall(glBindBuffer( type , m_bufferID ));

            #if GL_DEBUG == 1
            //std::cout << m_bufferID << " Constructed with type: " << GetEnum<type>() << std::endl;
            #endif


            m_size = 0;
        }
        BufferObject(uint id , uint size) {
            m_bufferID = id;
            m_size = size;
        }
        BufferObject(const void* const data , uint32_t size) {
            GLCall(glGenBuffers( 1 , &m_bufferID ));
            GLCall(glBindBuffer( type , m_bufferID ));
            GLCall(glBufferData( type , size , data , usage ));

            #if GL_DEBUG == 1
            //std::cout << m_bufferID << " Constructed with type: " << GetEnum<type>() << std::endl;
            #endif

            m_size = size;
        }
        BufferObject( BufferObject<type , usage>&& other ) {

            #if GL_DEBUG == 1
            std::cout << other.m_bufferID << " moved with type: " << GetEnum<type>() << std::endl;
            #endif

            m_bufferID = other.m_bufferID;
            m_size = other.m_size;
            other.m_bufferID = 0;
        }

        ~BufferObject() {
            #if GL_DEBUG == 1
            //std::cout << m_bufferID << " Distructed with type: " << GetEnum<type>() << std::endl;
            #endif
            GLCall(glDeleteBuffers(1, &m_bufferID));
        }
        void Swap( BufferObject<type , usage>& other ) {
            std::swap(m_size , other.m_size);
            std::swap(m_bufferID , other.m_bufferID);
        }

        void Invalidate(uint32_t offset , uint32_t size) {
            GLCall(glBindBuffer(type, m_bufferID));
            GLCall(glInvalidateBufferSubData(type , offset , size));
        }
        void ImmutableStorage(const void* data , uint32_t size ,GLbitfield storageFlags ) {
            m_size = size;
            GLCall(glBindBuffer(type, m_bufferID));
            GLCall(glBufferStorage(type , m_size , data , storageFlags ));
        }

        void* ImmutableMap(const void* data , uint32_t size , GLbitfield storageFlags , GLbitfield mapFlags) {

            m_size = size;
            GLCall(glBindBuffer(type, m_bufferID));
            GLCall(glBufferStorage(type , m_size , data , storageFlags ));
            GLCall(void* r = glMapBufferRange( type , 0 , m_size , mapFlags ));
            return r;
        }

        void* ImmutableResizeMap(const void* data , uint32_t size , GLbitfield storageFlags , GLbitfield mapFlags) {
            m_size = size;
            GLCall(glDeleteBuffers(1, &m_bufferID));
            GLCall(glGenBuffers( 1 , &m_bufferID ));

            GLCall(glBindBuffer(type, m_bufferID));
            GLCall(glBufferStorage(type , m_size , data , storageFlags ));
            GLCall(void* r = glMapBufferRange( type , 0 , m_size , mapFlags ));
            return r;
        }

        void* ImmutableResizeCopyMap(uint32_t size , GLbitfield storageFlags , GLbitfield mapFlags ) {

            uint32_t newBuffer;
            GLCall(glGenBuffers(1, &newBuffer));
            GLCall(glBindBuffer(type, m_bufferID));
            GLCall(glBufferStorage(type , size , nullptr , storageFlags ));
            GLCall(glCopyNamedBufferSubData(m_bufferID , newBuffer , 0 , 0  , std::min(m_size , size) ));
            GLCall(glDeleteBuffers(1 , &m_bufferID));
            m_bufferID = newBuffer;
            m_size = size;
            GLCall(void* r = glMapBufferRange( type , 0 , m_size , mapFlags ));
            return r;
        }

        void* ImmutableCopyMap( const BufferObject<type>& other , GLbitfield storageFlags , GLbitfield mapFlags ) {

            uint32_t newBuffer;
            GLCall(glGenBuffers(1, &newBuffer));
            GLCall(glBindBuffer(type, m_bufferID));
            GLCall(glBufferStorage(type , other.m_size , nullptr , storageFlags ));
            GLCall(glCopyNamedBufferSubData(other.m_bufferID , newBuffer , NULL , NULL , other.m_size ));
            GLCall(glDeleteBuffers(1 , &m_bufferID));
            m_bufferID = newBuffer;
            m_size = other.m_size;
            GLCall(void* r = glMapBufferRange( type , 0 , m_size , mapFlags ));
            return r;
        }

        void* Map(uint32_t size , uint32_t offsetPtr , GLbitfield access ) {
            GLCall(glBindBuffer(type, m_bufferID));
            GLCall(void* r = glMapBufferRange(type , offsetPtr , size , access));
            return r;
        }

        void Flush( uint32_t size , uint32_t offsetPtr ) {
            GLCall(glFlushMappedBufferRange( type, offsetPtr , size ));
        }

        void UnMap() {
            GLCall(glBindBuffer(type, m_bufferID));
            GLCall(glUnmapBuffer(type));
        }

        void FillBuffer(uint32_t size , uint32_t offset) {
            GLCall(glBindBuffer( type , m_bufferID ));
            GLCall(glBufferSubData( type , offset , size , nullptr ));
        }
        void FillBuffer(void* data , uint32_t size , uint32_t offset) {
            GLCall(glBindBuffer( type , m_bufferID ));
            GLCall(glBufferSubData( type , offset , size , data ));
        }

        void ResizeBuffer(uint32_t size) {
            GLCall(glBindBuffer( type , m_bufferID ));
            GLCall(glBufferData( type , size , nullptr , usage ));

            m_size = size;
        }

        void ResizeBuffer(void* data , uint32_t size) {
            GLCall(glBindBuffer( type , m_bufferID ));
            GLCall(glBufferData( type , size , data, usage ));

            m_size = size;
        }

        // void AddShader( Shader& shader , const std::string&& name ) const {
            
        //     static_assert( type == GL_UNIFORM_BUFFER , "Buffer needs to be of type GL_UNIFORM_BUFFER" );

        //     GLint bindingPoint;
        //     GLCall(uint32_t index = glGetUniformBlockIndex(shader.GetShaderID() , name.c_str() ));

        //     GLCall(glGetActiveUniformBlockiv(shader.GetShaderID() , index , GL_UNIFORM_BLOCK_BINDING , &bindingPoint ));
        //     GLCall(glBindBufferBase( type , bindingPoint , m_bufferID ));
        // }
        void BindBase(const int bindingPoint) const {
            static_assert( type == GL_UNIFORM_BUFFER || type == GL_SHADER_STORAGE_BUFFER || type == GL_ATOMIC_COUNTER_BUFFER, "Buffer needs to be of type GL_UNIFORM_BUFFER" );
            GLCall(glBindBufferBase( type , bindingPoint , m_bufferID ));
        }

        void Bind() const {
            GLCall(glBindBuffer( type , m_bufferID ));
        }
        void UnBind() const {
            GLCall(glBindBuffer( type , 0 ));
        }

        uint32_t GetID() const {
            return m_bufferID;
        }

        uint32_t GetSize() const {
            return m_size;
        }
        void SetSize(const uint size) {
            m_size = size;
        }

    private:
        uint32_t m_bufferID;
        uint32_t m_size;
};