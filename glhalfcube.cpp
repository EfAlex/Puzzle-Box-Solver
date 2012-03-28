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


#include "glhalfcube.hpp"
#include "GL/glut.h"
#include <boost/numeric/ublas/io.hpp>
#include <iostream>
#include "glwidget.hpp"

glhalfcube::glhalfcube():globject() {
    solid[0]=
        solid[1]=
        solid[2]=
        solid[3]=0.5;

    wired[0]=
        wired[1]=
        wired[2]=1.0;
    wired[3]=0.5;

    direction.resize(2);
    direction[0].resize(3);
    direction[1].resize(3);

    direction[0][0]=
        direction[0][1]=
        direction[0][2]=
        direction[1][0]=
        direction[1][1]=
        direction[1][2]=0;
}

void glhalfcube::draw()
{
    //std::cout << "glhalfcube draw" << std::endl;
    glPushMatrix();

    glTranslatef(pos[0],pos[1],pos[2]);

    GLfloat m[16] = {
        1,0,0,0,
        0,1,0,0,
        0,0,1,0,
        0,0,0,1
    };
    //std::cout << "draw direction[0]: " << direction[0] << " direction[1]: " << direction[1] << std::endl;
    m[0] = 1.0 * direction[0][0];
    m[1] = 1.0 * direction[0][1];
    m[2] = 1.0 * direction[0][2];

    m[4] = 1.0 * direction[1][0];
    m[5] = 1.0 * direction[1][1];
    m[6] = 1.0 * direction[1][2];

    /*std::cout << "matrix: " << m[0] << " " << m[4] << " " << m[8] << " " << m[12] << std::endl;
    std::cout << "        " << m[1] << " " << m[5] << " " << m[9] << " " << m[13] << std::endl;
    std::cout << "        " << m[2] << " " << m[6] << " " << m[10] << " " << m[14] << std::endl;
    std::cout << "        " << m[3] << " " << m[7] << " " << m[11] << " " << m[15] << std::endl;*/

    GLfloat *x1 = &m[0];
    GLfloat *y1 = &m[1];
    GLfloat *z1 = &m[2];
    GLfloat *x2 = &m[4];
    GLfloat *y2 = &m[5];
    GLfloat *z2 = &m[6];
    GLfloat *x3 = &m[8];
    GLfloat *y3 = &m[9];
    GLfloat *z3 = &m[10];
    *x3 = *y1 * *z2 - *z1 * *y2;
    *y3 = *z1 * *x2 - *x1 * *z2;
    *z3 = *x1 * *y2 - *y1 * *x2;

    /*std::cout << "matrix: " << m[0] << " " << m[4] << " " << m[8] << " " << m[12] << std::endl;
    std::cout << "        " << m[1] << " " << m[5] << " " << m[9] << " " << m[13] << std::endl;
    std::cout << "        " << m[2] << " " << m[6] << " " << m[10] << " " << m[14] << std::endl;
    std::cout << "        " << m[3] << " " << m[7] << " " << m[11] << " " << m[15] << std::endl;*/

    glMultMatrixf(m);

    GLboolean lighting;

    glGetBooleanv(GL_LIGHTING, &lighting);

    if (lighting)
        glDisable(GL_LIGHTING);

    if (GLWidget::useAlpha) {
        glColor4f(wired[0], wired[1], wired[2], wired[3]);
    } else {
        glColor3f(wired[0], wired[1], wired[2]);
    }
    wiredHalfCube();

    if (lighting)
        glEnable(GL_LIGHTING);

    if (GLWidget::useAlpha) {
        glColor4f(solid[0], solid[1], solid[2], solid[3]);
    } else {
        glColor3f(solid[0], solid[1], solid[2]);
    }
    solidHalfCube();
    glPopMatrix();

    /*glColor4f(wired[0], wired[1], wired[2], wired[3]);
    glPushMatrix();

    glTranslatef(pos[0],pos[1],pos[2]);
    glBegin(GL_LINES);

    glVertex3f(0, 0, 0);
    glVertex3f(0.5 * (direction[0][0]), 0.5 * (direction[0][1]), 0.5 * (direction[0][2]));
    glVertex3f(0, 0, 0);
    glVertex3f(0.5 * (direction[1][0]), 0.5 * (direction[1][1]), 0.5 * (direction[1][2]));

    glEnd();
    glPopMatrix();*/
}

void glhalfcube::solidHalfCube()
{
    glBegin(GL_QUADS);

    glNormal3f(0,-1,0);
    glVertex3f(0.5, -0.5, -0.5);
    glVertex3f(0.5, -0.5, 0.5);
    glVertex3f(-0.5, -0.5, 0.5);
    glVertex3f(-0.5, -0.5, -0.5);

    glNormal3f(-1,0,0);
    glVertex3f(-0.5, -0.5, -0.5);
    glVertex3f(-0.5, -0.5, 0.5);
    glVertex3f(-0.5, 0.5, 0.5);
    glVertex3f(-0.5, 0.5, -0.5);

    glNormal3f(0.707106781187f,0.707106781187f,0);
    glVertex3f(0.5, -0.5, 0.5);
    glVertex3f(0.5, -0.5, -0.5);
    glVertex3f(-0.5, 0.5, -0.5);
    glVertex3f(-0.5, 0.5, 0.5);

    glEnd();

    glBegin(GL_TRIANGLES);

    glNormal3f(0,0,-1);
    glVertex3f(-0.5, 0.5, -0.5);
    glVertex3f(0.5, -0.5, -0.5);
    glVertex3f(-0.5, -0.5, -0.5);

    glNormal3f(0,0,1);
    glVertex3f(-0.5, -0.5, 0.5);
    glVertex3f(0.5, -0.5, 0.5);
    glVertex3f(-0.5, 0.5, 0.5);

    glEnd();
}

void glhalfcube::wiredHalfCube()
{
    GLboolean lighting;

    glGetBooleanv(GL_LIGHTING, &lighting);

    if (lighting)
        glDisable(GL_LIGHTING);

    glBegin(GL_LINES);

    glVertex3f(-0.5, -0.5, -0.5);
    glVertex3f(-0.5, -0.5, 0.5);
    glVertex3f(-0.5, -0.5, 0.5);
    glVertex3f(0.5, -0.5, 0.5);
    glVertex3f(0.5, -0.5, 0.5);
    glVertex3f(0.5, -0.5, -0.5);
    glVertex3f(0.5, -0.5, -0.5);
    glVertex3f(-0.5, -0.5, -0.5);

    glVertex3f(-0.5, -0.5, 0.5);
    glVertex3f(-0.5, 0.5, 0.5);
    glVertex3f(-0.5, 0.5, 0.5);
    glVertex3f(-0.5, 0.5, -0.5);
    glVertex3f(-0.5, 0.5, -0.5);
    glVertex3f(-0.5, -0.5, -0.5);

    glVertex3f(0.5, -0.5, -0.5);
    glVertex3f(-0.5, 0.5, -0.5);
    glVertex3f(0.5, -0.5, 0.5);
    glVertex3f(-0.5, 0.5, 0.5);

    glEnd();
    if (lighting)
        glEnable(GL_LIGHTING);
}

glhalfcube * glhalfcube::do_clone()
{
    return new glhalfcube(*this);
}

