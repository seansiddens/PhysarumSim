//
// Created by sean on 9/7/21.
//

// TODO: Rewrite this to use C++ features

#include "Shader.h"

#include <iostream>
#include <cstdio>
#include <cstring>

Shader::Shader(const char *vertexPath, const char *fragmentPath) {
    // 1. Retrieve the vertex/fragment source code from filePath
    FILE *vertexFile = fopen(vertexPath, "r");
    FILE *fragmentFile = fopen(fragmentPath, "r");
    if ((vertexFile == nullptr) || (fragmentFile == nullptr)) {
        std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_OPENED" << std::endl;
        std::cout << strerror(errno) << '\n';
    } else {
        char *vShaderCode;
        char *fShaderCode;
        size_t vFileLength;
        size_t fFileLength;
        size_t vReadLength;
        size_t fReadLength;

        // Get length of vertex shader file
        fseek(vertexFile, 0, SEEK_END);
        vFileLength = ftell(vertexFile);
        fseek(vertexFile, 0, SEEK_SET);

        // Get length of fragment shader file
        fseek(fragmentFile, 0, SEEK_END);
        fFileLength = ftell(fragmentFile);
        fseek(fragmentFile, 0, SEEK_SET);

        // Allocate buffers to store shader source code
        vShaderCode = (char *) malloc(vFileLength + 1);
        fShaderCode = (char *) malloc(fFileLength + 1);

        // Read code from files and store in buffer
        vReadLength = fread(vShaderCode, 1, vFileLength, vertexFile);
        fReadLength = fread(fShaderCode, 1, fFileLength, fragmentFile);
        if ((vReadLength != vFileLength) || (fReadLength != fFileLength)) {
            free(vShaderCode);
            free(fShaderCode);
            std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ" << std::endl;
        } else {
            // Put escape char at end of string
            vShaderCode[vFileLength] = '\0';
            fShaderCode[fFileLength] = '\0';

            // 2. Compile shaders
            unsigned int vertex, fragment;
            int success;
            char infoLog[512];
            // Vertex Shader
            vertex = glCreateShader(GL_VERTEX_SHADER);
            glShaderSource(vertex, 1, &vShaderCode, NULL);
            glCompileShader(vertex);
            // print compile errors if any
            glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(vertex, 512, NULL, infoLog);
                std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
            }
            // Fragment Shader
            fragment = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(fragment, 1, &fShaderCode, NULL);
            glCompileShader(fragment);
            // print compile errors if any
            glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(fragment, 512, NULL, infoLog);
                std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
            }

            // Link shader Program
            ID = glCreateProgram();
            glAttachShader(ID, vertex);
            glAttachShader(ID, fragment);
            glLinkProgram(ID);
            // print linking errors if any
            glGetProgramiv(ID, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(ID, 512, NULL, infoLog);
                std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
            }

            // delete the shaders as they're linked into our program now and no longer necessary
            glDeleteShader(vertex);
            glDeleteShader(fragment);
        }
    }
}

void Shader::use() const {
    glUseProgram(ID);
}

void Shader::uniform2f(const char *name, float v0, float v1) const {
    glUniform2f(glGetUniformLocation(ID, name), v0, v1);
}

ComputeShader::ComputeShader(const char *computePath) {
    FILE *computeFile= fopen(computePath, "r");
    if (computePath == nullptr) {
        std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_OPENED" << std::endl;
        std::cout << strerror(errno) << '\n';
    } else {
        char *computeCode;
        size_t fileLength;
        size_t readLength;

        fseek(computeFile, 0, SEEK_END);
        fileLength = ftell(computeFile);
        fseek(computeFile, 0, SEEK_SET);

        // Allocate buffers to store shader source code
        computeCode = (char *) malloc(fileLength + 1);

        // Read code from files and store in buffer
        readLength = fread(computeCode, 1, fileLength, computeFile);
        if (readLength != fileLength) {
            free(computeCode);
            std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ" << std::endl;
        } else {
            // Put escape char at end of string
            computeCode[fileLength] = '\0';

            // 2. Compile shader
            unsigned int compute;
            int success;
            char infoLog[512];
            compute = glCreateShader(GL_COMPUTE_SHADER);
            glShaderSource(compute, 1, &computeCode, NULL);
            glCompileShader(compute);
            // print compile errors if any
            glGetShaderiv(compute, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(compute, 512, NULL, infoLog);
                std::cout << "ERROR::SHADER::COMPUTE::COMPILATION_FAILED\n" << infoLog << std::endl;
            }

            // Link shader Program
            ID = glCreateProgram();
            glAttachShader(ID, compute);
            glLinkProgram(ID);
            // print linking errors if any
            glGetProgramiv(ID, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(ID, 512, NULL, infoLog);
                std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
            }

            // delete the shaders as they're linked into our program now and no longer necessary
            free(computeCode);
            glDeleteShader(compute);
        }
    }
}

void ComputeShader::use() const {
    glUseProgram(ID);
}

void ComputeShader::uniform1f(const char *name, float v0) const {
    glUniform1f(glGetUniformLocation(ID, name), v0);
}

void ComputeShader::uniform1ui(const char *name, unsigned int v0) const {
    glUniform1ui(glGetUniformLocation(ID, name), v0);
}

void ComputeShader::uniform2f(const char *name, float v0, float v1) const {
    glUniform2f(glGetUniformLocation(ID, name), v0, v1);
}
