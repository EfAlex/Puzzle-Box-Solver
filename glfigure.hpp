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


#ifndef GLFIGURE_HPP
#define GLFIGURE_HPP

#include "common.hpp"
#include "globject.hpp"
#include <GL/gl.h>
#include <vector>
#include "figure.hpp"

class glfigure: public globject
{
public:
    GLfloat solid[4], wired[4];

    glfigure();
    glfigure(const figure &, const GLfloat solidc[4], const GLfloat wiredc[4]);
    virtual void draw();
    virtual glfigure *do_clone();
private:
    figure f;
};

#endif // GLFIGURE_HPP
