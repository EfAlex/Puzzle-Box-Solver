/*
   Copyright 2012 efalex <email>

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#ifndef COREGL_HPP
#define COREGL_HPP

#include <QOpenGLFunctions>       // base GL functions (initialize, etc.)
#include <qopenglextrafunctions.h>// extra GL functions (VAO/VBO) — must be explicit include in Qt5
#include <QOpenGLShaderProgram>   // shader compilation/linking
#include <QString>                // for shader file paths
#include <QMatrix4x4>
#include <QVector3D>
#include <QtMath>                 // qFuzzyCompare, etc.
#include <QOpenGLVertexArrayObject> // For Qt5 VAO management
#include <QOpenGLBuffer>            // For Qt5 VBO management

// Maximum number of instances we can handle
#define MAX_INSTANCES 1024

class CoreGL : protected QOpenGLExtraFunctions {
public:
    void init();                    // compile/link shaders; create VAO/VBOs for cube+halfcube geo
    bool isValid() const;           // true if shader program loaded successfully
    /* For instanced rendering */
    void setCubeInstanceData(const float* positions, const float* colors, const float* rotations, int count);
    void setHalfCubeInstanceData(const float* positions, const float* colors, const float* rotations, int count);
    void drawCubes(int count);              // Instanced drawing of cubes
    void drawHalfCubes(int count);          // Instanced drawing of halfcubes

    /* Store P and V separately */
    void setMatricesPv(const QMatrix4x4 &pv, const QMatrix4x4 &v);
    void setModelMatrix(const QMatrix4x4 &model);


    void setColors(float solid_r, float solid_g, float solid_b);
    void drawCube();                // Legacy: one call per cube
    void drawHalfCube(int dirIdx);  // Legacy: one call per halfcube

    /* Scale factor handling */
    void setScaleFactor(float scale) { scaleFactor_ = scale; }
    float getScaleFactor() const { return scaleFactor_; }

    /* Debug mode */
    void setDebugMode(int debug) { debugMode_ = debug; }

    /* Light position (from Tetris3D: configurable light direction) */
    void setLightPosition(float x, float y, float z) {
        lightPos_[0] = x; lightPos_[1] = y; lightPos_[2] = z;
        // Note: do NOT call setUniformValue here — the shader program is not bound.
        // The uniform is applied in drawCubes()/drawHalfCubes() where the program IS bound.
    }

    /* Rendering mode (from Tetris3D: 0 = neon, 1 = solid) */
    void setRenderingMode(int mode) {
        renderingMode_ = mode;
        // Uniform applied in drawCubes()/drawHalfCubes() where program IS bound
    }

    /* Neon intensity (from Tetris3D) */
    void setNeonIntensity(float intensity) {
        neonIntensity_ = intensity;
        // Uniform applied in drawCubes()/drawHalfCubes() where program IS bound
    }

    /* Instance data arrays for nested objects */
    float cube_positions[3 * MAX_INSTANCES];   // 3 floats per position (x, y, z)
    float cube_colors[4 * MAX_INSTANCES];      // 4 floats per color (r, g, b, a)
    float cube_rotations[9 * MAX_INSTANCES];   // 9 floats per rotation matrix (3x3)
    int cube_count;                             // Current number of cubes

    float halfcube_positions[3 * MAX_INSTANCES]; // 3 floats per position (x, y, z)
    float halfcube_colors[4 * MAX_INSTANCES];    // 4 floats per color (r, g, b, a)
    float halfcube_rotations[9 * MAX_INSTANCES]; // 9 floats per rotation matrix (3x3)
    int halfcube_count;                          // Current number of halfcubes

    /* Methods to reset counters and collect instance data */
    void resetInstanceCounters();
    void collectInstanceDataFromObjects(const float* positions, const float* colors, const float* rotations, int count, bool isCube);

    /* Helper functions for matrix operations */
    void printMatrix4x4(const float* matrix, const char* label);

    /* Public matrices for direct access by objects */
    QMatrix4x4 projMat_;            // Projection matrix
    QMatrix4x4 viewMat_;            // View matrix
    QMatrix4x4 modelMat_;           // Global model rotation matrix

    ~CoreGL() { cleanup(); }

private:
    void cleanup();
    bool compileShader(const QString &path, const QOpenGLShader::ShaderType type, const char **sourceOut);

    QOpenGLShaderProgram shaderProg_;

    /* For instanced rendering - Cubes */
    QOpenGLBuffer cube_pos_vbo_;
    QOpenGLBuffer cube_color_vbo_;
    QOpenGLBuffer cube_rot_vbo_;


    /* For instanced rendering - HalfCubes */
    QOpenGLBuffer halfcube_pos_vbo_;
    QOpenGLBuffer halfcube_color_vbo_;
    QOpenGLBuffer halfcube_rot_vbo_;

    /* Base Geometries */
    QOpenGLVertexArrayObject cube_vao_;
    QOpenGLBuffer cube_vbo_;
    int    cube_vert_count_ = 0;

    QOpenGLVertexArrayObject halfcube_vao_;
    QOpenGLBuffer halfcube_vbo_;
    int    halfcube_vert_count_ = 0;

    float solid_r_, solid_g_, solid_b_;

    /* Scale factor */
    float scaleFactor_ = 1.0f;      // Default scale factor

    /* Debug mode flag */
    int debugMode_ = 0;  // 0 = normal, 1 = instance pos, 2 = vertex pos, 3 = vertex normal

    /* Light position uniform */
    float lightPos_[3] = {0.5f, 1.0f, 0.3f};

    /* Rendering mode: 0 = neon glow, 1 = solid lit */
    int renderingMode_ = 0;

    /* Neon glow intensity */
    float neonIntensity_ = 1.0f;
};

#endif // COREGL_HPP
