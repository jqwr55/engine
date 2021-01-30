#pragma once

#include <Debug.h>

class BindlessTexture {
    public:
        BindlessTexture() = default;
        ~BindlessTexture();
        void Init();

        void SetTextureParameters(GLenum pname[] , const GLint params[] , uint32_t size);
        template<const unsigned int type> void Bind() {
            GLCall(glBindTexture(type , ID));
        }
        template<const unsigned int type> void ImmutableStorage(const GLenum internalFormat , const unsigned int mipLevel , int x, int y, int z) {
            GLCall(glBindTexture(type , ID));
            GLCall(glTexStorage3D(type , mipLevel , internalFormat , x,y,z));
        }
        template<const unsigned int type> void Upload(void* data , const GLenum dataFormat , const GLenum dataType , const unsigned int mipLevel , int x , int y , int z) {
            GLCall(glBindTexture(type , ID));
            GLCall(glTexSubImage3D(type , mipLevel , 0,0,0, x,y,z , dataFormat , dataType , data));
        }
        uint64_t GetAddress();
        
        uint32_t ID = 0;
};

template<> inline void BindlessTexture::ImmutableStorage<GL_TEXTURE_1D>(const GLenum internalFormat , const unsigned int mipLevel , int x, int , int z) {
    GLCall(glBindTexture(GL_TEXTURE_1D , ID));
    GLCall(glTexStorage1D(GL_TEXTURE_1D , mipLevel , internalFormat , x));
}
template<> inline void BindlessTexture::ImmutableStorage<GL_TEXTURE_1D_ARRAY>(const GLenum internalFormat , const unsigned int mipLevel , int x, int y, int z) {
    GLCall(glBindTexture(GL_TEXTURE_1D_ARRAY , ID));
    GLCall(glTexStorage2D(GL_TEXTURE_1D , mipLevel , internalFormat , x,y));
}
template<> inline void BindlessTexture::ImmutableStorage<GL_TEXTURE_2D>(const GLenum internalFormat , const unsigned int mipLevel , int x, int y, int z) {
    GLCall(glBindTexture(GL_TEXTURE_2D , ID));
    GLCall(glTexStorage2D(GL_TEXTURE_2D , mipLevel , internalFormat , x,y));
}
template<> inline void BindlessTexture::ImmutableStorage<GL_TEXTURE_2D_ARRAY>(const GLenum internalFormat , const unsigned int mipLevel , int x, int y, int z) {
    GLCall(glBindTexture(GL_TEXTURE_2D_ARRAY , ID));
    GLCall(glTexStorage3D(GL_TEXTURE_2D_ARRAY , mipLevel , internalFormat , x,y,z));
}
template<> inline void BindlessTexture::ImmutableStorage<GL_TEXTURE_3D>(const GLenum internalFormat , const unsigned int mipLevel , int x, int y, int z) {
    GLCall(glBindTexture(GL_TEXTURE_3D , ID));
    GLCall(glTexStorage3D(GL_TEXTURE_3D , mipLevel , internalFormat , x,y,z));
}

template<> inline void BindlessTexture::Upload<GL_TEXTURE_1D>(void* data , const GLenum dataFormat , const GLenum dataType , const unsigned int mipLevel , int x , int y , int z) {
    GLCall(glBindTexture(GL_TEXTURE_1D , ID));
    GLCall(glTexSubImage1D(GL_TEXTURE_1D , mipLevel , 0,x,dataFormat,dataType,data));
}
template<> inline void BindlessTexture::Upload<GL_TEXTURE_1D_ARRAY>(void* data , const GLenum dataFormat , const GLenum dataType , const unsigned int mipLevel , int x , int y , int z) {
    GLCall(glBindTexture(GL_TEXTURE_1D_ARRAY , ID));
    GLCall(glTexSubImage2D(GL_TEXTURE_1D_ARRAY , mipLevel , 0,0,x,y,dataFormat,dataType,data));
}
template<> inline void BindlessTexture::Upload<GL_TEXTURE_2D>(void* data , const GLenum dataFormat , const GLenum dataType , const unsigned int mipLevel , int x , int y , int z) {
    GLCall(glBindTexture(GL_TEXTURE_2D , ID));
    GLCall(glTexSubImage2D(GL_TEXTURE_2D , mipLevel , 0,0,x,y,dataFormat,dataType,data));
}
template<> inline void BindlessTexture::Upload<GL_TEXTURE_2D_ARRAY>(void* data , const GLenum dataFormat , const GLenum dataType , const unsigned int mipLevel , int x , int y , int z) {
    GLCall(glBindTexture(GL_TEXTURE_2D_ARRAY , ID));
    GLCall(glTexSubImage3D(GL_TEXTURE_2D_ARRAY , mipLevel , 0,0,0 ,x,y,z, dataFormat,dataType,data));
}
template<> inline void BindlessTexture::Upload<GL_TEXTURE_3D>(void* data , const GLenum dataFormat , const GLenum dataType , const unsigned int mipLevel , int x , int y , int z) {
    GLCall(glBindTexture(GL_TEXTURE_3D , ID));
    GLCall(glTexSubImage3D(GL_TEXTURE_3D , mipLevel , 0,0,0 ,x,y,z, dataFormat,dataType,data));
}