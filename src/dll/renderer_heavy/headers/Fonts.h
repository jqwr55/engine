#pragma once

#include <fstream>
#include <iostream>
#include <string>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/scalar_multiplication.hpp>

struct FontCharMeta {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    int xOffset = 0;
    int yOffset = 0;
    int xAdvance = 0;
    int page = 0;
    int chnl = 0;
    bool valid = false;
};

struct FontCharMetaFast {
    int16_t xOffset;
    int16_t yOffset;
    int16_t xAdvance;
};

struct Vertex {
    glm::vec2 vertexPosition;
    glm::vec2 vertexTexCoord;
};

struct alignas(64) Quad {
    Vertex vertecies[4];
    static unsigned int indecies[6];
    Quad operator * ( glm::vec4 transform ) const;
};

struct FontMeta {
    std::string fileName;
    std::string infoFace;
    std::string charSet;
    
    int size;
    int bold;
    int italic;
    int unicode;
    int stretchH;
    int smooth;
    int aa;
    int padding[4];
    int spacing[2];
    int commonLineHeight;
    int base;
    int scaleW;
    int scaleH;
    int pages;
    int packed;
    int pageId;
    int charCount;

    int kerningsCount;
    int kerningFirst;
    int kerningSecond;
    int kerningAmount;

    int maxWidth = 0;
    int maxHeight = 0;
};

struct Font {
    FontMeta meta;
    FontCharMeta* charMetaSet = nullptr;
    FontCharMetaFast* charMetaSetFast = nullptr;
    Quad* charQuadSet = nullptr;
};