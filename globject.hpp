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


#ifndef GLOBJECT_HPP
#define GLOBJECT_HPP

#include "common.hpp"
#include "GL/gl.h"
#include <boost/ptr_container/ptr_vector.hpp>

class globject;

class globject
{
public:
    GLfloat pos[3];

    globject();
    virtual void draw();
    globject *new_clone(globject &);
    globject *clone();
    virtual globject *do_clone();
protected:
    boost::ptr_vector < globject > objects;
};

#endif // GLOBJECT_HPP
