#include <TextureObject.h>

BindlessTexture::~BindlessTexture() {
    GLCall(glDeleteTextures(1, &ID));
    ID = 0;
}

uint64_t BindlessTexture::GetAddress() {
    GLCall(return glGetTextureHandleARB(ID));;
}

void BindlessTexture::Init() {
    GLCall(glGenTextures(1, &ID));
}

void BindlessTexture::SetTextureParameters(GLenum pname[] , const GLint params[] , uint32_t size) {
    for(int i = 0; i < size ; i++) {
        GLCall(glTextureParameteri(ID , pname[i] , params[i]));
    }
}