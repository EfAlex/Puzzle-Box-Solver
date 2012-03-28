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


#ifndef FIGURE_HPP
#define FIGURE_HPP

#include "common.hpp"
#include <boost/numeric/ublas/matrix.hpp>
#include "cube.hpp"
#include "figure_cube.hpp"
#include <boost/ptr_container/ptr_vector.hpp>

class figure {
public:
    boost::ptr_vector < figure_cube > cubes;

    figure();
    figure(const figure &);
    figure & operator=(const figure &);

    vector_int direction[3];
    void reset();
    void addFigureCube(figure_cube);
    void rotate(boost::numeric::ublas::matrix < int >);
    figure *new_clone();
};

#endif // FIGURE_HPP
