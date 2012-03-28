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


#include "glcube.hpp"
#include "GL/glut.h"
#include <iostream>
#include "glwidget.hpp"

glcube::glcube():globject() {
    solid[0]=
        solid[1]=
            solid[2]=
                solid[3]=0.5;

    wired[0]=
        wired[1]=
            wired[2]=1.0;
    wired[3]=0.5;
}

void glcube::draw()
{
    //std::cout << "glcube draw" << std::endl;
    glPushMatrix();
    glTranslatef(pos[0],pos[1],pos[2]);
    GLboolean lighting;

    glGetBooleanv(GL_LIGHTING, &lighting);

    if (lighting)
        glDisable(GL_LIGHTING);

    if (GLWidget::useAlpha) {
        glColor4f(wired[0], wired[1], wired[2], wired[3]);
    } else {
        glColor3f(wired[0], wired[1], wired[2]);
    }
    glutWireCube(1.0);

    if (lighting)
        glEnable(GL_LIGHTING);

    if (GLWidget::useAlpha) {
        glColor4f(solid[0], solid[1], solid[2], solid[3]);
    } else {
        glColor3f(solid[0], solid[1], solid[2]);
    }
    glutSolidCube(1.0);

    glPopMatrix();
}

glcube * glcube::do_clone()
{
    return new glcube(*this);
}

