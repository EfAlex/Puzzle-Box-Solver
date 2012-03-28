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


#ifndef FIGURE_CUBE_HPP
#define FIGURE_CUBE_HPP

#include "common.hpp"
#include "cube.hpp"

class figure_cube {
public:
    cube c;
    int pos[3];
    figure_cube();
    figure_cube(const figure_cube &);
    figure_cube & operator=(const figure_cube &);
    figure_cube* new_clone(figure_cube &) const;
};

#endif // FIGURE_CUBE_HPP
