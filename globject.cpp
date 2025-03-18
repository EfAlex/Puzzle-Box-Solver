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


#include "globject.hpp"
//#include <iostream>
#include <boost/foreach.hpp>

globject::~globject() = default;  // virtual destructor for polymorphic deletion safety

globject::globject():coregl_(nullptr), parent_(nullptr), combinedMatrixValid_(false)
{
    pos[0] = pos[1] = pos[2] = 0;
    modelMatrix_.setToIdentity();
    objects.resize(0);
}

void globject::draw()
{
    updateLocalMatrix();

    BOOST_FOREACH( globject &o, objects) {
        o.draw();
    }
}

globject * globject::new_clone(globject &other)
{
    return other.clone();
}

globject * globject::clone()
{
    return do_clone();
}

globject * globject::do_clone()
{
    return new globject(*this);
}

/* Propagate coregl_ to self AND all nested children (recursive) */
void globject::setCoreGL(CoreGL *coregl) {
    coregl_ = coregl;
    BOOST_FOREACH(globject &o, objects) {
        o.setCoreGL(coregl);  // cascade down the entire tree
    }
}

void globject::setParent(globject* parent)
{
    parent_ = parent;
}

void globject::updateLocalMatrix()
{
    modelMatrix_.setToIdentity();
    modelMatrix_.translate(pos[0], pos[1], pos[2]);
    combinedMatrixValid_ = false;  // Invalidate cached matrix when local matrix changes
}

globject* globject::getParent() const
{
    return parent_;
}

const QMatrix4x4& globject::computeCombinedMatrix() const
{
    // If the cached matrix is valid, return it
    if (combinedMatrixValid_) {
        return combinedMatrix_;
    }

    // Compute the combined matrix
    combinedMatrix_ = modelMatrix_;  // Start with local transformation

    // Apply parent transformations if we have a parent
    if (parent_ != nullptr) {
        const QMatrix4x4& parentCombined = parent_->computeCombinedMatrix();
        combinedMatrix_ = parentCombined * combinedMatrix_;
    }

    // Mark the matrix as valid
    combinedMatrixValid_ = true;

    return combinedMatrix_;
}

