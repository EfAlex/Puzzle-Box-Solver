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
#include <boost/foreach.hpp>
#include "globject.hpp"
#include "glfigure.hpp"
#include "glcube.hpp"
#include "glhalfcube.hpp"
#include "coregl.hpp"

using namespace boost::numeric::ublas;

glsolution::glsolution() {
    objects.resize(0);
    coregl_ = nullptr;
}

glsolution::glsolution(unsigned int s, const std::vector < figure > & sol, const std::vector < vector_int > & sol_pos)
{
    static const GLfloat figColors[][4] = {
        { 0.5, 0.0, 0.0, 0.2 },
        { 0.0, 0.5, 0.0, 0.2 },
        { 0.0, 0.0, 0.5, 0.2 },
        { 0.3, 0.3, 0.0, 0.2 },
        { 0.0, 0.3, 0.3, 0.2 },
        { 0.3, 0.0, 0.3, 0.2 },
        { 0.2, 0.2, 0.2, 0.2 },
        { 0.3, 0.4, 0.4, 0.2 }
    };

    objects.resize(0);
    coregl_ = nullptr;

    size_t count = std::min<size_t>(s, std::min(sol.size(), sol_pos.size()));

    for (unsigned int i = 0; i < count; ++i) {
        glfigure *f = new glfigure(sol[i], figColors[i]);
        f->pos[0] = sol_pos[i][0] - 1.5f;
        f->pos[1] = sol_pos[i][1] - 1.0f;
        f->pos[2] = sol_pos[i][2] - 0.5f;
        objects.push_back(f);
    }
}

/* Propagate coregl_ pointer down through all child objects */
void glsolution::propagateCoreGL(CoreGL *coregl) {
    coregl_ = coregl;
    BOOST_FOREACH(globject &o, objects) {
        o.setCoreGL(coregl);
    }
}

void glsolution::draw() {
    modelMatrix_.setToIdentity();
    modelMatrix_.translate(pos[0], pos[1], pos[2]);
    combinedMatrixValid_ = false;

    BOOST_FOREACH(globject &o, objects) {
        o.draw();
    }
}

glsolution * glsolution::do_clone()
{
    return new glsolution(*this);
}

