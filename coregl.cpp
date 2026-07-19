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

#include "coregl.hpp"
#include <QCoreApplication>
#include <QFile>
#include <QDebug>

/* ------------------------------------------------------------------ */
/*  Shader loading / compilation                                        */
/* ------------------------------------------------------------------ */

bool CoreGL::compileShader(const QString &path, const QOpenGLShader::ShaderType type, const char **sourceOut)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "CoreGL: cannot open shader" << path;
        return false;
    }

    QString src = QString(f.readAll());
    const char *data = qPrintable(src);
    *sourceOut = data;

    if (!shaderProg_.addShaderFromSourceCode(type, src)) {
        qWarning() << "CoreGL:" << (type == QOpenGLShader::Vertex ? "vertex" : "fragment")
                   << "compile error:" << shaderProg_.log();
        return false;
    }

    return true;
}

/* ------------------------------------------------------------------ */
/*  init — compile shaders + build base geometries                    */
/* ------------------------------------------------------------------ */

void CoreGL::init()
{
    initializeOpenGLFunctions();

    /* ---- shader compilation / linking ---- */
    QString appDir = QCoreApplication::applicationDirPath();
    QStringList candidatePaths;
    candidatePaths << appDir + "/../shaders"
                   << appDir + "/shaders";

    const char *vsSource, *fsSource;
    bool ok = false;
    QString lastError;

    for (const auto &base : candidatePaths) {
        QString vsPath  = base + "/shader_vs.vert";
        QString fsPath  = base + "/shader_fs.frag";

        if (!QFile::exists(vsPath) || !QFile::exists(fsPath)) continue;

        if (compileShader(vsPath, QOpenGLShader::Vertex, &vsSource) &&
            compileShader(fsPath,  QOpenGLShader::Fragment, &fsSource)) {
            if (shaderProg_.link() && shaderProg_.isLinked()) {
                ok = true;
                break;
            }
        }
    }

    if (!ok) {
        qCritical("CoreGL: could not find or compile shaders.");
        return;
    }

    /* ================================================================ */
    /*  Base Cube Geometry (Expanded to 36 vertices for glDrawArrays)  */
    /* ================================================================ */
    struct Vertex { float pos[3]; float normal[3]; float uv[2]; };
    static const Vertex cubeVerts[] = {
        // Front (+Z): uv from XY
        {{-0.5f,-0.5f, 0.5f},{ 0.f, 0.f, 1.f},{0.f,0.f}}, {{ 0.5f,-0.5f, 0.5f},{ 0.f, 0.f, 1.f},{1.f,0.f}}, {{-0.5f, 0.5f, 0.5f},{ 0.f, 0.f, 1.f},{0.f,1.f}},
        {{ 0.5f,-0.5f, 0.5f},{ 0.f, 0.f, 1.f},{1.f,0.f}}, {{ 0.5f, 0.5f, 0.5f},{ 0.f, 0.f, 1.f},{1.f,1.f}}, {{-0.5f, 0.5f, 0.5f},{ 0.f, 0.f, 1.f},{0.f,1.f}},
        // Back (-Z): uv from XY
        {{ 0.5f,-0.5f,-0.5f},{ 0.f, 0.f,-1.f},{0.f,0.f}}, {{-0.5f,-0.5f,-0.5f},{ 0.f, 0.f,-1.f},{1.f,0.f}}, {{ 0.5f, 0.5f,-0.5f},{ 0.f, 0.f,-1.f},{0.f,1.f}},
        {{-0.5f,-0.5f,-0.5f},{ 0.f, 0.f,-1.f},{1.f,0.f}}, {{-0.5f, 0.5f,-0.5f},{ 0.f, 0.f,-1.f},{1.f,1.f}}, {{ 0.5f, 0.5f,-0.5f},{ 0.f, 0.f,-1.f},{0.f,1.f}},
        // Top (+Y): uv from XZ
        {{-0.5f, 0.5f,-0.5f},{ 0.f, 1.f, 0.f},{0.f,0.f}}, {{ 0.5f, 0.5f,-0.5f},{ 0.f, 1.f, 0.f},{1.f,0.f}}, {{-0.5f, 0.5f, 0.5f},{ 0.f, 1.f, 0.f},{0.f,1.f}},
        {{ 0.5f, 0.5f,-0.5f},{ 0.f, 1.f, 0.f},{1.f,0.f}}, {{ 0.5f, 0.5f, 0.5f},{ 0.f, 1.f, 0.f},{1.f,1.f}}, {{-0.5f, 0.5f, 0.5f},{ 0.f, 1.f, 0.f},{0.f,1.f}},
        // Bottom (-Y): uv from XZ
        {{-0.5f,-0.5f, 0.5f},{ 0.f,-1.f, 0.f},{0.f,0.f}}, {{ 0.5f,-0.5f, 0.5f},{ 0.f,-1.f, 0.f},{1.f,0.f}}, {{-0.5f,-0.5f,-0.5f},{ 0.f,-1.f, 0.f},{0.f,1.f}},
        {{ 0.5f,-0.5f, 0.5f},{ 0.f,-1.f, 0.f},{1.f,0.f}}, {{ 0.5f,-0.5f,-0.5f},{ 0.f,-1.f, 0.f},{1.f,1.f}}, {{-0.5f,-0.5f,-0.5f},{ 0.f,-1.f, 0.f},{0.f,1.f}},
        // Left (-X): uv from YZ
        {{-0.5f,-0.5f,-0.5f},{-1.f, 0.f, 0.f},{0.f,0.f}}, {{-0.5f,-0.5f, 0.5f},{-1.f, 0.f, 0.f},{1.f,0.f}}, {{-0.5f, 0.5f,-0.5f},{-1.f, 0.f, 0.f},{0.f,1.f}},
        {{-0.5f,-0.5f, 0.5f},{-1.f, 0.f, 0.f},{1.f,0.f}}, {{-0.5f, 0.5f, 0.5f},{-1.f, 0.f, 0.f},{1.f,1.f}}, {{-0.5f, 0.5f,-0.5f},{-1.f, 0.f, 0.f},{0.f,1.f}},
        // Right (+X): uv from YZ
        {{ 0.5f,-0.5f, 0.5f},{ 1.f, 0.f, 0.f},{0.f,0.f}}, {{ 0.5f,-0.5f,-0.5f},{ 1.f, 0.f, 0.f},{1.f,0.f}}, {{ 0.5f, 0.5f, 0.5f},{ 1.f, 0.f, 0.f},{0.f,1.f}},
        {{ 0.5f,-0.5f,-0.5f},{ 1.f, 0.f, 0.f},{1.f,0.f}}, {{ 0.5f, 0.5f,-0.5f},{ 1.f, 0.f, 0.f},{1.f,1.f}}, {{ 0.5f, 0.5f, 0.5f},{ 1.f, 0.f, 0.f},{0.f,1.f}},
    };

    cube_vao_.create();
    cube_vao_.bind();
    cube_vbo_.create();
    cube_vbo_.bind();
    cube_vbo_.allocate(cubeVerts, sizeof(cubeVerts));
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);
    // UV attribute for edge detection (Tetris3D-inspired)
    glVertexAttribPointer(7, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
    glEnableVertexAttribArray(7);
    cube_vert_count_ = 36;

    /* ================================================================ */
    /*  Base HalfCube Geometry (x+y <= 0 cut of unit cube)             */
    /*  5 faces: bottom(square), left(square), diagonal(rect),         */
    /*  back(tri), front(tri) — total 24 vertices                      */
    /* ================================================================ */
    static const Vertex hcVerts[] = {
        // Bottom face (Y=-0.5): full square, normal = (0,-1,0)
        {{-0.5f,-0.5f,-0.5f},{ 0.f,-1.f, 0.f},{0.f,0.f}}, {{ 0.5f,-0.5f,-0.5f},{ 0.f,-1.f, 0.f},{1.f,0.f}}, {{-0.5f,-0.5f, 0.5f},{ 0.f,-1.f, 0.f},{0.f,1.f}},
        {{ 0.5f,-0.5f,-0.5f},{ 0.f,-1.f, 0.f},{1.f,0.f}}, {{ 0.5f,-0.5f, 0.5f},{ 0.f,-1.f, 0.f},{1.f,1.f}}, {{-0.5f,-0.5f, 0.5f},{ 0.f,-1.f, 0.f},{0.f,1.f}},
        // Left face (X=-0.5): full square, normal = (-1,0,0)
        {{-0.5f,-0.5f,-0.5f},{-1.f, 0.f, 0.f},{0.f,0.f}}, {{-0.5f,-0.5f, 0.5f},{-1.f, 0.f, 0.f},{1.f,0.f}}, {{-0.5f, 0.5f,-0.5f},{-1.f, 0.f, 0.f},{0.f,1.f}},
        {{-0.5f,-0.5f, 0.5f},{-1.f, 0.f, 0.f},{1.f,0.f}}, {{-0.5f, 0.5f, 0.5f},{-1.f, 0.f, 0.f},{1.f,1.f}}, {{-0.5f, 0.5f,-0.5f},{-1.f, 0.f, 0.f},{0.f,1.f}},
        // Diagonal plane (x+y=0): rectangle, normal = (1,1,0)/sqrt(2) — CCW from outside
        {{ 0.5f,-0.5f, 0.5f},{ 1.f, 1.f, 0.f},{0.f,0.f}}, {{-0.5f, 0.5f, 0.5f},{ 1.f, 1.f, 0.f},{1.f,0.f}}, {{-0.5f, 0.5f,-0.5f},{ 1.f, 1.f, 0.f},{1.f,1.f}},
        {{ 0.5f,-0.5f, 0.5f},{ 1.f, 1.f, 0.f},{0.f,0.f}}, {{-0.5f, 0.5f,-0.5f},{ 1.f, 1.f, 0.f},{1.f,1.f}}, {{ 0.5f,-0.5f,-0.5f},{ 1.f, 1.f, 0.f},{0.f,1.f}},
        // Back face (Z=-0.5): triangle for x+y<=0, CCW from outside (normal = (0,0,-1))
        {{-0.5f, 0.5f,-0.5f},{ 0.f, 0.f,-1.f},{0.f,1.f}}, {{-0.5f,-0.5f,-0.5f},{ 0.f, 0.f,-1.f},{0.f,0.f}}, {{ 0.5f,-0.5f,-0.5f},{ 0.f, 0.f,-1.f},{1.f,0.f}},
        // Front face (Z=0.5): triangle, normal = (0,0,1)
        {{-0.5f,-0.5f, 0.5f},{ 0.f, 0.f, 1.f},{0.f,0.f}}, {{-0.5f, 0.5f, 0.5f},{ 0.f, 0.f, 1.f},{0.f,1.f}}, {{ 0.5f,-0.5f, 0.5f},{ 0.f, 0.f, 1.f},{1.f,0.f}},
    };

    halfcube_vao_.create();
    halfcube_vao_.bind();
    halfcube_vbo_.create();
    halfcube_vbo_.bind();
    halfcube_vbo_.allocate(hcVerts, sizeof(hcVerts));
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);
    // UV attribute for edge detection (Tetris3D-inspired)
    glVertexAttribPointer(7, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
    glEnableVertexAttribArray(7);
    halfcube_vert_count_ = 24;

}

/* ------------------------------------------------------------------ */
/*  Matrix management                                                  */
/* ------------------------------------------------------------------ */

void CoreGL::setMatricesPv(const QMatrix4x4 &pv, const QMatrix4x4 &v) {
    projMat_ = pv;
    viewMat_ = v;
}

void CoreGL::setModelMatrix(const QMatrix4x4 &model) {
    modelMat_ = model;
}


/* ------------------------------------------------------------------ */
/*  isValid check                                                      */
/* ------------------------------------------------------------------ */

bool CoreGL::isValid() const {
    return shaderProg_.isLinked();
}

/* ------------------------------------------------------------------ */
/*  Uniform management                                                 */
/* ------------------------------------------------------------------ */

void CoreGL::setColors(float solid_r, float solid_g, float solid_b) {
    solid_r_ = solid_r; solid_g_ = solid_g; solid_b_ = solid_b;
}

/* ------------------------------------------------------------------ */
/*  Instanced rendering data setup                                     */
/* ------------------------------------------------------------------ */

void CoreGL::setCubeInstanceData(const float* positions, const float* colors, const float* rotations, int count) {
    if (count <= 0) return;

    cube_vao_.bind();

    if (cube_pos_vbo_.isCreated() == false) cube_pos_vbo_.create();
    cube_pos_vbo_.bind();
    cube_pos_vbo_.allocate(positions, sizeof(float) * 3 * count);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);

    if (cube_color_vbo_.isCreated() == false) cube_color_vbo_.create();
    cube_color_vbo_.bind();
    cube_color_vbo_.allocate(colors, sizeof(float) * 4 * count);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 0, 0);  // vec4 RGBA
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1);

    if (cube_rot_vbo_.isCreated() == false) cube_rot_vbo_.create();
    cube_rot_vbo_.bind();
    cube_rot_vbo_.allocate(rotations, sizeof(float) * 9 * count);

    for (int i = 0; i < 3; ++i) {
        glEnableVertexAttribArray(4 + i);
        glVertexAttribPointer(4 + i, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 9, (void*)(sizeof(float) * 3 * i));
        glVertexAttribDivisor(4 + i, 1);
    }

    cube_vao_.release();
}

void CoreGL::setHalfCubeInstanceData(const float* positions, const float* colors, const float* rotations, int count) {
    if (count <= 0) return;

    halfcube_vao_.bind();

    if (halfcube_pos_vbo_.isCreated() == false) halfcube_pos_vbo_.create();
    halfcube_pos_vbo_.bind();
    halfcube_pos_vbo_.allocate(positions, sizeof(float) * 3 * count);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);

    if (halfcube_color_vbo_.isCreated() == false) halfcube_color_vbo_.create();
    halfcube_color_vbo_.bind();
    halfcube_color_vbo_.allocate(colors, sizeof(float) * 4 * count);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 0, 0);  // vec4 RGBA
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1);

    if (halfcube_rot_vbo_.isCreated() == false) halfcube_rot_vbo_.create();
    halfcube_rot_vbo_.bind();
    halfcube_rot_vbo_.allocate(rotations, sizeof(float) * 9 * count);

    // Rotation matrix as 3 vec3 attributes (locations 4, 5, 6)
    for (int i = 0; i < 3; ++i) {
        glEnableVertexAttribArray(4 + i);
        glVertexAttribPointer(4 + i, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 9, (void*)(sizeof(float) * 3 * i));
        glVertexAttribDivisor(4 + i, 1);
    }

    halfcube_vao_.release();
}

/* ------------------------------------------------------------------ */
/*  Instance data collection                                           */
/* ------------------------------------------------------------------ */

void CoreGL::resetInstanceCounters() {
    cube_count = 0;
    halfcube_count = 0;
}

void CoreGL::collectInstanceDataFromObjects(const float* positions, const float* colors, const float* rotations, int count, bool isCube) {
    if (count <= 0) return;

    if (isCube) {
        // Check bounds
        if (cube_count + count > MAX_INSTANCES) {
            qWarning() << "CoreGL::collectInstanceDataFromObjects - Cube instance limit exceeded!";
            return;
        }

        // Copy position data
        for (int i = 0; i < count * 3; i++) {
            cube_positions[(cube_count * 3) + i] = positions[i];
        }

        // Copy color data (RGBA: 4 floats per instance)
        for (int i = 0; i < count * 4; i++) {
            cube_colors[(cube_count * 4) + i] = colors[i];
        }

        // Copy rotation data
        for (int i = 0; i < count * 9; i++) {
            cube_rotations[(cube_count * 9) + i] = rotations[i];
        }

        cube_count += count;
    } else {
        // Check bounds
        if (halfcube_count + count > MAX_INSTANCES) {
            qWarning() << "CoreGL::collectInstanceDataFromObjects - Halfcube instance limit exceeded!";
            return;
        }

        // Copy position data
        for (int i = 0; i < count * 3; i++) {
            halfcube_positions[(halfcube_count * 3) + i] = positions[i];
        }

        // Copy color data (RGBA: 4 floats per instance)
        for (int i = 0; i < count * 4; i++) {
            halfcube_colors[(halfcube_count * 4) + i] = colors[i];
        }

        // Copy rotation data
        for (int i = 0; i < count * 9; i++) {
            halfcube_rotations[(halfcube_count * 9) + i] = rotations[i];
        }

        halfcube_count += count;
    }
}

/* ------------------------------------------------------------------ */
/*  Draw calls                                                         */
/* ------------------------------------------------------------------ */

void CoreGL::drawCubes(int count) {
    if (count <= 0) return;

    shaderProg_.bind();

    // Validate instance data count
    if (!cube_pos_vbo_.isCreated() || !cube_color_vbo_.isCreated() || !cube_rot_vbo_.isCreated()) {
        qWarning() << "ERROR: Instance data not set up properly for cubes";
        return;
    }

    shaderProg_.bind();
    shaderProg_.setUniformValue("u_proj", projMat_);
    shaderProg_.setUniformValue("u_view", viewMat_);
    shaderProg_.setUniformValue("u_model", modelMat_);
    shaderProg_.setUniformValue("u_draw_axis", 0);

    // Pass debug mode flag to shader
    shaderProg_.setUniformValue("u_debug_mode", debugMode_ ? 1 : 0);

    // Pass Tetris3D-inspired uniforms (light, rendering mode, neon intensity)
    shaderProg_.setUniformValue("u_lightPosition", lightPos_[0], lightPos_[1], lightPos_[2]);
    shaderProg_.setUniformValue("u_renderingMode", renderingMode_);
    shaderProg_.setUniformValue("u_neonIntensity", neonIntensity_);

    cube_vao_.bind();
    glDrawArraysInstanced(GL_TRIANGLES, 0, cube_vert_count_, count);
    cube_vao_.release();
    shaderProg_.release();
}

void CoreGL::drawHalfCubes(int count) {
    if (count <= 0) return;

    shaderProg_.bind();

    // Validate instance data count
    if (!halfcube_pos_vbo_.isCreated() || !halfcube_color_vbo_.isCreated() || !halfcube_rot_vbo_.isCreated()) {
        qWarning() << "ERROR: Instance data not set up properly for halfcubes";
        return;
    }

    shaderProg_.bind();
    shaderProg_.setUniformValue("u_proj", projMat_);
    shaderProg_.setUniformValue("u_view", viewMat_);
    shaderProg_.setUniformValue("u_model", modelMat_);
    shaderProg_.setUniformValue("u_draw_axis", 0);

    // Pass debug mode flag to shader
    shaderProg_.setUniformValue("u_debug_mode", debugMode_ ? 1 : 0);

    // Pass Tetris3D-inspired uniforms (light, rendering mode, neon intensity)
    shaderProg_.setUniformValue("u_lightPosition", lightPos_[0], lightPos_[1], lightPos_[2]);
    shaderProg_.setUniformValue("u_renderingMode", renderingMode_);
    shaderProg_.setUniformValue("u_neonIntensity", neonIntensity_);

    halfcube_vao_.bind();
    glDrawArraysInstanced(GL_TRIANGLES, 0, halfcube_vert_count_, count);
    halfcube_vao_.release();
    shaderProg_.release();
}

/* ------------------------------------------------------------------ */
/*  Helper functions for matrix operations                             */
/* ------------------------------------------------------------------ */



void CoreGL::cleanup() {
    cube_vao_.destroy();
    cube_vbo_.destroy();
    cube_pos_vbo_.destroy();
    cube_color_vbo_.destroy();
    cube_rot_vbo_.destroy();

    halfcube_vao_.destroy();
    halfcube_vbo_.destroy();
    halfcube_pos_vbo_.destroy();
    halfcube_color_vbo_.destroy();
    halfcube_rot_vbo_.destroy();
}
