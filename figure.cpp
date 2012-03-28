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


#include "figure.hpp"
#include "figure_cube.hpp"
#include <boost/foreach.hpp>
#include <iostream>

using namespace boost::numeric::ublas;

figure::figure()
{
    reset();
}
void figure::reset()
{
    cubes.resize(0);
    direction[0].resize(3);
    direction[1].resize(3);
    direction[0][1] = direction[0][2] =
                          direction[1][0] = direction[1][2] =
                                                                      0;
    direction[0][0] = direction[1][1] = 1;
}

figure::figure(const figure & other) {
    //std::cout << "Figure copy" << std::endl;
    cubes.resize(0);
    direction[0].resize(3);
    direction[1].resize(3);

    direction[0][0] = other.direction[0][0];
    direction[0][1] = other.direction[0][1];
    direction[0][2] = other.direction[0][2];
    direction[1][0] = other.direction[1][0];
    direction[1][1] = other.direction[1][1];
    direction[1][2] = other.direction[1][2];

    BOOST_FOREACH(const figure_cube & c, other.cubes) {
        figure_cube *p = new figure_cube(c);
        cubes.push_back(p);
    }

}

figure &figure::operator=(const figure & other) {
    //std::cout << "Figure operator=" << std::endl;
    if (this == &other) {
        return *this;
    }
    cubes.resize(0);
    direction[0].resize(3);
    direction[1].resize(3);

    direction[0][0] = other.direction[0][0];
    direction[0][1] = other.direction[0][1];
    direction[0][2] = other.direction[0][2];
    direction[1][0] = other.direction[1][0];
    direction[1][1] = other.direction[1][1];
    direction[1][2] = other.direction[1][2];

    BOOST_FOREACH(const figure_cube & c, other.cubes) {
        figure_cube *p = new figure_cube(c);
        cubes.push_back(p);
    }


    return *this;
}

void figure::addFigureCube(figure_cube i)
{
    figure_cube *p = new figure_cube(i);
    cubes.push_back(p);
}

void figure::rotate(matrix < int >m)
{
    BOOST_FOREACH(figure_cube & i, cubes) {
        vector_int p (3);
        p[0] = i.pos[0];
        p[1] = i.pos[1];
        p[2] = i.pos[2];
        p = prod(m, p);
        i.pos[0] = p[0];
        i.pos[1] = p[1];
        i.pos[2] = p[2];
        int *orig = i.c.getVector();
        vector_int v (3);
        v[0] = orig[0];
        v[1] = orig[1];
        v[2] = orig[2];
        vector_int res (3);
        res = prod(m, v);
        int r[3];
        r[0] = res[0];
        r[1] = res[1];
        r[2] = res[2];
        i.c.setVector(r);
    }
    direction[0] = prod(m, direction[0]);
    direction[1] = prod(m, direction[1]);
}
figure *figure::new_clone()
{
    return new figure(*this);
}
