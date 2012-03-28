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


#include "figure_cube.hpp"
#include <iostream>
#include <boost/numeric/ublas/io.hpp>

figure_cube::figure_cube()
{
}

figure_cube::figure_cube(const figure_cube & other)
{
    //std::cout << "Figure_cube copy" << std::endl;
//    std::cout << other.pos << std::endl;
    pos[0] = other.pos[0];
    pos[1] = other.pos[1];
    pos[2] = other.pos[2];
    c = other.c;
}

figure_cube & figure_cube::operator=(const figure_cube &other)
{
    //std::cout << "Figure_cube operator=" << std::endl;
    if (this == &other) {
        return *this;
    }
    pos[0] = other.pos[0];
    pos[1] = other.pos[1];
    pos[2] = other.pos[2];
    c = other.c;

    return *this;
}
figure_cube* figure_cube::new_clone(figure_cube &other) const
{
    return new figure_cube(other);
}
