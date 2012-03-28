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


#ifndef GLSOLUTION_HPP
#define GLSOLUTION_HPP

#include "common.hpp"
#include "globject.hpp"
#include <vector>
#include "figure.hpp"

class glsolution: public globject
{
public:
    std::vector < figure > solution;
    float scale;

    glsolution();
    glsolution(unsigned int s, const std::vector < figure > & , const std::vector < vector_int > & );
    virtual void draw();
    virtual glsolution *do_clone();
};

#endif // GLSOLUTION_HPP
