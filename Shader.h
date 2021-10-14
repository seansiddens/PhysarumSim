//
// Created by sean on 9/7/21.
//

#ifndef LEARNOPENGL_SHADER_H
#define LEARNOPENGL_SHADER_H

#include <GL/glew.h>

class Shader {
public:
    // The Program ID
    unsigned int ID;

    // Constructor reads and builds the shader
    Shader(const char *vertexPath, const char *fragmentPath);

    // Use/enable shader program
    void use() const;

    // Utility functions for setting uniforms
    void uniform2f(const char *name, float v0, float v1) const;
};

class ComputeShader {
public:
    unsigned int ID;

    explicit ComputeShader(const char *computePath);

    void use() const;

    void uniform1f(const char *name, float v0) const;
    void uniform1ui(const char *name, unsigned int v0) const;
    void uniform2f(const char*name, float v0, float v1) const;
};


#endif //LEARNOPENGL_SHADER_H
