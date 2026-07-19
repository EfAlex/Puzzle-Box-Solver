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
#include <boost/foreach.hpp>
#include "globject.hpp"
#include "glcube.hpp"
#include "glhalfcube.hpp"

using namespace boost::numeric::ublas;

glfigure::glfigure() {
    solid[0]=
        solid[1]=
            solid[2]=
                solid[3]=0.5;

    pos[0] =
        pos[1] =
            pos[2] = 0;

    objects.resize(0);
}

glfigure::glfigure(const figure & figur, const GLfloat solidc[4])
{
    objects.resize(0);
    pos[0] = pos[1] = pos[2] = 0;

    solid[0] = solidc[0];
    solid[1] = solidc[1];
    solid[2] = solidc[2];
    solid[3] = solidc[3];

    f = figur;

    for(unsigned int fci = 0; fci < f.n_cubes; ++fci) {
        figure_cube fc = f.cubes[fci];
        if (fc.c.isFull()) {
            glcube *glc = new glcube();
            glc->solid[0] = solid[0];
            glc->solid[1] = solid[1];
            glc->solid[2] = solid[2];
            glc->solid[3] = solid[3];
            glc->pos[0] = fc.pos[0];
            glc->pos[1] = fc.pos[1];
            glc->pos[2] = fc.pos[2];
            objects.push_back(glc);
            glc->setParent(this);
        } else if (fc.c.isHalf()) {
            glhalfcube *glh = new glhalfcube();
            glh->solid[0] = solid[0];
            glh->solid[1] = solid[1];
            glh->solid[2] = solid[2];
            glh->solid[3] = solid[3];
            glh->pos[0] = fc.pos[0];
            glh->pos[1] = fc.pos[1];
            glh->pos[2] = fc.pos[2];
            int *v = fc.c.getVector();
            if (v[0] == 0) {
                glh->direction[0][0] = glh->direction[0][2] = glh->direction[1][0] = glh->direction[1][1] = 0;
                glh->direction[0][1] = v[1];
                glh->direction[1][2] = v[2];
            } else if (v[1] == 0) {
                glh->direction[0][1] = glh->direction[0][2] = glh->direction[1][0] = glh->direction[1][1] = 0;
                glh->direction[0][0] = v[0];
                glh->direction[1][2] = v[2];
            } else if (v[2] == 0) {
                glh->direction[0][1] = glh->direction[0][2] = glh->direction[1][0] = glh->direction[1][2] = 0;
                glh->direction[0][0] = v[0];
                glh->direction[1][1] = v[1];
            } else {
                std::cerr << "Half cube add error" << std::endl;
                exit(1);
            }
            objects.push_back(glh);
            glh->setParent(this);
        }
    }
}

void glfigure::draw()
{
    float scale = coregl_ ? coregl_->getScaleFactor() : 1.0f;
    float origX = pos[0], origY = pos[1], origZ = pos[2];
    pos[0] = origX * scale;
    pos[1] = origY * scale;
    pos[2] = origZ * scale;

    updateLocalMatrix();

    BOOST_FOREACH( globject &o, objects) {
        o.draw();
    }

    pos[0] = origX;
    pos[1] = origY;
    pos[2] = origZ;
}

glfigure * glfigure::do_clone()
{
    return new glfigure(*this);
}

