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


#include "glsolution.hpp"
#include <GL/glut.h>
#include <boost/foreach.hpp>
#include "globject.hpp"
#include "glfigure.hpp"
#include <boost/numeric/ublas/io.hpp>
#include <iostream>

using namespace boost::numeric::ublas;

glsolution::glsolution() {
    objects.resize(0);
}

glsolution::glsolution(unsigned int s, const std::vector < figure > & sol, const std::vector < vector_int > & sol_pos)
{
    const GLfloat colors[2][8][4] = {
        {
            { 0.5, 0.0, 0.0, 0.2 },
            { 0.0, 0.5, 0.0, 0.2 },
            { 0.0, 0.0, 0.5, 0.2 },
            { 0.3, 0.3, 0.0, 0.2 },
            { 0.0, 0.3, 0.3, 0.2 },
            { 0.3, 0.0, 0.3, 0.2 },
            { 0.2, 0.2, 0.2, 0.2 },
            { 0.3, 0.4, 0.4, 0.2 }
        },
        {
            { 1.0, 0.0, 0.0, 0.4 },
            { 0.0, 1.0, 0.0, 0.4 },
            { 0.0, 0.0, 1.0, 0.4 },
            { 0.6, 0.6, 0.0, 0.4 },
            { 0.0, 0.6, 0.6, 0.4 },
            { 0.6, 0.0, 0.6, 0.4 },
            { 0.4, 0.4, 0.4, 0.4 },
            { 0.6, 0.8, 0.8, 0.4 }
        }
    };

    objects.resize(0);


    for (unsigned int i = 0; i < s; ++i) {
        /*std::cout << sol_pos[i] << std::endl;
        figure ff = sol[s];

        for( unsigned int fci = 0 ; fci < ff.cubes.size(); ++fci) {
            std::cout << ff.cubes[fci].pos << " " << ff.cubes[fci].c.getState() << std::endl;
        }*/
        glfigure *f = new glfigure(sol[i], colors[0][i] , colors[1][i]);
        f->pos[0] = sol_pos[i][0];
        f->pos[1] = sol_pos[i][1];
        f->pos[2] = sol_pos[i][2];
        objects.push_back(f);
    }
}

void glsolution::draw()
{
    //std::cout << "glsolution draw" << std::endl;
    BOOST_FOREACH(globject &o, objects) {
        glPushMatrix();
        glTranslatef((o.pos[0] - 1.5) * scale, (o.pos[1] - 1.0) * scale, (o.pos[2] - 0.5) * scale);
        o.draw();
        glPopMatrix();
    }
}

glsolution * glsolution::do_clone()
{
    return new glsolution(*this);
}

