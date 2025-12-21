// generate white plane
#pragma once

#include <glad/gl.h>
#include <vector>
#include "model.h"

inline Model* createWhiteFloor() {
    Model* m = new Model();

    // All white plane
    GLuint whiteTexture;
    glGenTextures(1, &whiteTexture);
    glBindTexture(GL_TEXTURE_2D, whiteTexture);
    unsigned char whitePixel[] = {255, 255, 255}; // RGB (全白)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, whitePixel);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    m->textures.push_back(whiteTexture);
    float size = 500.0f;
    float y_bias = -0.01f;

    float pos[] = {
        -size, y_bias, -size, // 左下
        -size, y_bias,  size, // 左上
         size, y_bias,  size, // 右上
         size, y_bias, -size  // 右下
    };

    float nor[] = {
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f
    };

    float tx[] = {
        0.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 1.0f,
        1.0f, 0.0f
    };

    for (int i = 0; i < 12; i++) {
        m->positions.push_back(pos[i]);
        m->normals.push_back(nor[i]);
    }
    for (int i = 0; i < 8; i++) {
        m->texcoords.push_back(tx[i]);
    }

    m->numVertex = 4;
    m->drawMode = GL_QUADS; 
    return m;
}