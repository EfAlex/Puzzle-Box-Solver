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
#include <cstring>

figure::figure()
{
    reset();
}
void figure::reset()
{
    n_cubes = 0;
    direction[0].resize(3);
    direction[1].resize(3);
    direction[0][1] = direction[0][2] =
                          direction[1][0] = direction[1][2] =
                                                                      0;
    direction[0][0] = direction[1][1] = 1;
}

figure::figure(const figure & other) {
    n_cubes = other.n_cubes;
    for (uint8_t i = 0; i < n_cubes; ++i) {
        cubes[i] = other.cubes[i];
    }
    direction[0].resize(3);
    direction[1].resize(3);

    direction[0][0] = other.direction[0][0];
    direction[0][1] = other.direction[0][1];
    direction[0][2] = other.direction[0][2];
    direction[1][0] = other.direction[1][0];
    direction[1][1] = other.direction[1][1];
    direction[1][2] = other.direction[1][2];
}

figure &figure::operator=(const figure & other) {
    if (this == &other) {
        return *this;
    }
    n_cubes = other.n_cubes;
    for (uint8_t i = 0; i < n_cubes; ++i) {
        cubes[i] = other.cubes[i];
    }
    direction[0].resize(3);
    direction[1].resize(3);

    direction[0][0] = other.direction[0][0];
    direction[0][1] = other.direction[0][1];
    direction[0][2] = other.direction[0][2];
    direction[1][0] = other.direction[1][0];
    direction[1][1] = other.direction[1][1];
    direction[1][2] = other.direction[1][2];

    return *this;
}

void figure::addFigureCube(figure_cube i)
{
    cubes[n_cubes++] = i;
}

void figure::rotate(const Mat3 &m)
{
    for (uint8_t ci = 0; ci < n_cubes; ++ci) {
        std::array<int, 3> p = {{ cubes[ci].pos[0], cubes[ci].pos[1], cubes[ci].pos[2] }};
        p = mat3_mul_vec(m, p);
        cubes[ci].pos[0] = p[0];
        cubes[ci].pos[1] = p[1];
        cubes[ci].pos[2] = p[2];
        if (cubes[ci].c.isHalf()) {
            const int *dv = cubes[ci].c.getVector();
            std::array<int, 3> v = {{ dv[0], dv[1], dv[2] }};
            auto res = mat3_mul_vec(m, v);
            cubes[ci].c.setVector(&res[0]);
        }
    }
    std::array<int, 3> d0 = {{ direction[0][0], direction[0][1], direction[0][2] }};
    std::array<int, 3> d1 = {{ direction[1][0], direction[1][1], direction[1][2] }};
    d0 = mat3_mul_vec(m, d0);
    d1 = mat3_mul_vec(m, d1);
    direction[0][0] = d0[0]; direction[0][1] = d0[1]; direction[0][2] = d0[2];
    direction[1][0] = d1[0]; direction[1][1] = d1[1]; direction[1][2] = d1[2];
}
figure *figure::new_clone()
{
    return new figure(*this);
}
