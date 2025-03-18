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

#include "glcube.hpp"
#include "globject.hpp"
#include <QMatrix4x4>
#include "coregl.hpp"
#include "glwidget.hpp"
#include <boost/foreach.hpp>

glcube::glcube():globject()
{
}

void glcube::draw()
{
    updateLocalMatrix();
    QMatrix4x4 combinedMatrix = computeCombinedMatrix();

    QVector3D position(combinedMatrix(0, 3), combinedMatrix(1, 3), combinedMatrix(2, 3));
    float positionArray[3] = {position.x(), position.y(), position.z()};

    float rotation[9] = { 1,0,0, 0,1,0, 0,0,1 };

    if (coregl_ != nullptr) {
        coregl_->collectInstanceDataFromObjects(positionArray, solid, rotation, 1, true);
    }

    BOOST_FOREACH( globject &o, objects) {
        o.draw();
    }
}

glcube * glcube::do_clone()
{
    return new glcube(*this);
}

