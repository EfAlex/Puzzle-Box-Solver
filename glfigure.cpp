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


#include "glfigure.hpp"
#include <GL/glut.h>
#include <boost/foreach.hpp>
#include "globject.hpp"
#include "glcube.hpp"
#include "glhalfcube.hpp"
#include <boost/numeric/ublas/io.hpp>
#include <iostream>

using namespace boost::numeric::ublas;

glfigure::glfigure() {
    solid[0]=
        solid[1]=
            solid[2]=
                solid[3]=0.5;

    wired[0]=
        wired[1]=
            wired[2]=1.0;
    wired[3]=0.5;

    pos[0] =
        pos[1] =
            pos[2] = 0;

    objects.resize(0);
}

glfigure::glfigure(const figure & figur, const GLfloat solidc[4], const GLfloat wiredc[4])
{
    //std::cout << "Start glfigure" << std::endl;
    objects.resize(0);
    //std::cout << "Start glfigure1" << std::endl;
    pos[0] =
        pos[1] =
        pos[2] = 0;

    solid[0]=solidc[0];
    solid[1]=solidc[1];
    solid[2]=solidc[2];
    solid[3]=solidc[3];

    wired[0]=wiredc[0];
    wired[1]=wiredc[1];
    wired[2]=wiredc[2];
    wired[3]=wiredc[3];

    //std::cout << "Start figur.cubes.size()" << figur.cubes.size() << std::endl;
    f = figur;
    //std::cout << "Start f.cubes.size()" << f.cubes.size() << std::endl;
    //std::cout << "Start glfigure2" << std::endl;

    //std::cout << "Start glfigure3" << std::endl;
    //std::cout << "Start f.cubes.size()" << f.cubes.size() << std::endl;

    for(unsigned int fci = 0; fci < f.cubes.size(); ++fci) {
        //for (std::vector < figure_cube >::iterator ci = f.cubes.begin(); ci != f.cubes.end(); ++ci) {
        //std::cout << "Start glfigure4" << std::endl;
        figure_cube fc = f.cubes[fci];
        //std::cout << "Start glfigure5" << std::endl;
        //std::cout << fc.c.isFull() << std::endl;
        if (fc.c.isFull()) {
            //std::cout << "Full" << fc.pos << std::endl;
            //std::cout << "Start new glcube" << std::endl;
            glcube *glc = new glcube();
            //std::cout << "End new glcube" << std::endl;
            glc->solid[0] = solid[0];
            glc->solid[1] = solid[1];
            glc->solid[2] = solid[2];
            glc->solid[3] = solid[3];
            glc->wired[0] = wired[0];
            glc->wired[1] = wired[1];
            glc->wired[2] = wired[2];
            glc->wired[3] = wired[3];
            glc->pos[0] = fc.pos[0];
            glc->pos[1] = fc.pos[1];
            glc->pos[2] = fc.pos[2];
            objects.push_back(glc);

        } else if (fc.c.isHalf()) {
            //std::cout << "Half " << fc.pos << " " << fc.c.getVector() << std::endl;
            //std::cout << "Start new glhalfcube" << std::endl;
            glhalfcube *glh = new glhalfcube();
            //std::cout << "End new glhalfcube" << std::endl;
            glh->solid[0] = solid[0];
            glh->solid[1] = solid[1];
            glh->solid[2] = solid[2];
            glh->solid[3] = solid[3];
            glh->wired[0] = wired[0];
            glh->wired[1] = wired[1];
            glh->wired[2] = wired[2];
            glh->wired[3] = wired[3];
            glh->pos[0] = fc.pos[0];
            glh->pos[1] = fc.pos[1];
            glh->pos[2] = fc.pos[2];
            int *v = fc.c.getVector();
            //std::cout << "v[0]: " << v[0] << " v[1]: " << v[1] << " v[2]: " << v[2] << std::endl;
            if (v[0] == 0) {
                glh->direction[0][0] =
                    glh->direction[0][2] =
                    glh->direction[1][0] =
                    glh->direction[1][1] = 0;
                glh->direction[0][1] = v[1];
                glh->direction[1][2] = v[2];
            } else if (v[1] == 0) {
                glh->direction[0][1] =
                    glh->direction[0][2] =
                    glh->direction[1][0] =
                    glh->direction[1][1] = 0;
                glh->direction[0][0] = v[0];
                glh->direction[1][2] = v[2];
            } else if (v[2] == 0) {
                glh->direction[0][1] =
                    glh->direction[0][2] =
                    glh->direction[1][0] =
                    glh->direction[1][2] = 0;
                glh->direction[0][0] = v[0];
                glh->direction[1][1] = v[1];
            } else {
                std::cerr << "Half cube add error" << std::endl;
                exit(1);
            }
            //std::cout << "glh->direction[0]: " << glh->direction[0] << " glh->direction[1]: " << glh->direction[1] << std::endl;
            objects.push_back(glh);
        }
    }
    //std::cout << std::endl;
    }

void glfigure::draw()
{
    //std::cout << "glfigure draw" << std::endl;
    glPushMatrix();

    /*GLfloat m[16] = {
        1,0,0,0,
        0,1,0,0,
        0,0,1,0,
        0,0,0,1
    };
    m[0] = direction[0][0];
    m[4] = direction[0][1];
    m[8] = direction[0][2];

    m[1] = direction[1][0];
    m[5] = direction[1][1];
    m[9] = direction[1][2];

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

    glMultMatrixf(m);*/

    //    glTranslatef(pos[0],pos[1],pos[2]);

    globject::draw();

    glPopMatrix();
}

glfigure * glfigure::do_clone()
{
    return new glfigure(*this);
}

