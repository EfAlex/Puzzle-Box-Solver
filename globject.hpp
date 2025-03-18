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
#include "coregl.hpp"
#include <QMatrix4x4>

class globject;

class globject
{
public:
    GLfloat pos[3];

    globject();
    ~globject();  // virtual destructor for polymorphic deletion via boost::ptr_vector
    virtual void draw();
    globject *new_clone(globject &);
    globject *clone();
    virtual globject *do_clone();
    virtual void setCoreGL(CoreGL *coregl);

    void setParent(globject* parent);
    globject* getParent() const;
    const QMatrix4x4& computeCombinedMatrix() const;
    void updateLocalMatrix();

protected:
    boost::ptr_vector < globject > objects;
    CoreGL *coregl_;
    QMatrix4x4 modelMatrix_;  // Local transformation matrix
    globject* parent_;        // Parent object in hierarchy
    mutable QMatrix4x4 combinedMatrix_;  // Cached combined transformation matrix
    mutable bool combinedMatrixValid_;   // Flag to track if cached matrix is valid
};

#endif // GLOBJECT_HPP
