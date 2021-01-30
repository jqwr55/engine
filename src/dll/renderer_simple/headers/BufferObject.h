#pragma once
#include <Debug.h>

template<u32 type> class BufferObject {
    public:
        BufferObject() {
            GLCall(glGenBuffers( 1 , &gl_handle ));
            GLCall(glBindBuffer( type , gl_handle ));
            m_size = 0;
        }
        BufferObject(u32 id , u32 size) {
            gl_handle = id;
            m_size = size;
        }
        ~BufferObject() {
            GLCall(glDeleteBuffers(1, &gl_handle));
        }
        void ImmutableStorage(const void* data , u32 size ,GLbitfield storageFlags ) {
            m_size = size;
            GLCall(glNamedBufferStorage(gl_handle , m_size , data , storageFlags ));
        }
        void ImmutableResize(const void* data , u32 size , GLbitfield storageFlags) {
            m_size = size;
            GLCall(glDeleteBuffers(1,&gl_handle));
            GLCall(glGenBuffers(1,&gl_handle));
            GLCall(glBindBuffer(type ,gl_handle));
            GLCall(glNamedBufferStorage(gl_handle , m_size , data , storageFlags ));
        }

        void* ImmutableMap(const void* data , u32 size , GLbitfield storageFlags , GLbitfield mapFlags) {
            m_size = size;
            GLCall(glNamedBufferStorage(gl_handle , m_size , data , storageFlags ));
            GLCall(void* r = glMapNamedBufferRange(gl_handle, 0 , m_size , mapFlags ));
            return r;
        }

        void* ImmutableResizeMap(const void* data , u32 size , GLbitfield storageFlags , GLbitfield mapFlags) {
            m_size = size;
            GLCall(glDeleteBuffers(1, &gl_handle));
            GLCall(glGenBuffers( 1 , &gl_handle ));
            GLCall(glBindBuffer(type, gl_handle));

            GLCall(glNamedBufferStorage(gl_handle , m_size , data , storageFlags ));
            GLCall(void* r = glMapNamedBufferRange(gl_handle, 0 , m_size , mapFlags ));
            return r;
        }
        void* ImmutableResizeCopyMap(u32 size , GLbitfield storageFlags , GLbitfield mapFlags ) {

            uint32_t newBuffer;
            GLCall(glGenBuffers(1, &newBuffer));
            GLCall(glBindBuffer(type, gl_handle));

            GLCall(glNamedBufferStorage(gl_handle , m_size , nullptr , storageFlags ));
            GLCall(glCopyNamedBufferSubData(gl_handle , newBuffer , 0 , 0  , std::min(m_size , size) ));
            GLCall(glDeleteBuffers(1 , &gl_handle));
            gl_handle = newBuffer;
            m_size = size;
            GLCall(void* r = glMapNamedBufferRange(gl_handle, 0 , m_size , mapFlags ));
            return r;
        }
        void* Map(u32 size , u32 offsetPtr , GLbitfield access ) {
            GLCall(void* r = glMapNamedBufferRange(gl_handle, 0 , m_size , access ));
            return r;
        }
        void Flush( u32 size , u32 offsetPtr ) {
            GLCall(glFlushMappedBufferRange( type, offsetPtr , size ));
        }
        void UnMap() {
            GLCall(glUnmapNamedBuffer(gl_handle));
        }
        void FillBufferZ(uint32_t size , uint32_t offset) {
            GLCall(glNamedBufferSubData(gl_handle , offset , size , nullptr));
        }
        void FillBufferD(void* data , uint32_t size , uint32_t offset) {
            GLCall(glNamedBufferSubData(gl_handle , offset , size , data));
        }
        void ResizeBufferCopy(u32 size) {
            u32 tmp;
            GLCall(glGenBuffers(1,&tmp));
            GLCall(glBindBuffer(type, tmp));
            GLCall(glNamedBufferData(tmp , size , nullptr , GL_DYNAMIC_DRAW));
            GLCall(glCopyNamedBufferSubData(gl_handle , tmp , 0,0,m_size));

            GLCall(glDeleteBuffers(1,&gl_handle));
            gl_handle = tmp;
            m_size = size;
        }
        void ResizeBufferZ(uint32_t size) {
            GLCall(glNamedBufferData( gl_handle, size , nullptr , GL_DYNAMIC_DRAW ));
            m_size = size;
        }
        void ResizeBufferD(void* data , u32 size) {
            GLCall(glNamedBufferData( gl_handle, size , data , GL_DYNAMIC_DRAW ));
            m_size = size;
        }
        void BindBase(const i32 bindingPoint) const {
            GLCall(glBindBufferBase( type , bindingPoint , gl_handle ));
        }
        void Bind() const {
            GLCall(glBindBuffer( type , gl_handle ));
        }
        void UnBind() const {
            GLCall(glBindBuffer( type , 0 ));
        }

        u32 gl_handle;
        u32 m_size;
};


constexpr GLbitfield PeristentStorage = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
constexpr GLbitfield PeristentMap = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;

template<const GLenum bufferType> struct PersistentlyMappedBuffer {

    PersistentlyMappedBuffer(u32 size , GLbitfield storageFlags = PeristentStorage , GLbitfield mapFlags = PeristentMap) {
        ptr = buffer.ImmutableMap(nullptr , size , storageFlags ,mapFlags );
    }
    void ResizeMap(u32 size ,GLbitfield storageFlags = PeristentStorage, GLbitfield mapFlags = PeristentMap) {
        ptr = buffer.ImmutableResizeMap(nullptr , size , PeristentStorage ,mapFlags );
    }

    BufferObject<bufferType> buffer;
    void* ptr = nullptr;
};
