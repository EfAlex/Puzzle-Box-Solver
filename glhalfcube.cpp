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


#include "glhalfcube.hpp"
#include <boost/foreach.hpp>
#include "glwidget.hpp"
#include "coregl.hpp"
#include <QMatrix4x4>

glhalfcube::glhalfcube():globject() {
    solid[0] = 0.5;
    solid[1] = 0.5;
    solid[2] = 0.5;

    direction.resize(2);
    direction[0].resize(3);
    direction[1].resize(3);

    direction[0][0] = 0;
    direction[0][1] = 0;
    direction[0][2] = 0;
    direction[1][0] = 0;
    direction[1][1] = 0;
    direction[1][2] = 0;
}

void glhalfcube::draw()
{
    updateLocalMatrix();
    QMatrix4x4 combinedMatrix = computeCombinedMatrix();

    QVector3D position(combinedMatrix(0, 3), combinedMatrix(1, 3), combinedMatrix(2, 3));
    float positionArray[3] = {position.x(), position.y(), position.z()};

    float rotation[9];
    if (!direction.empty() && direction.size() >= 2) {
        QVector3D dir0(direction[0][0], direction[0][1], direction[0][2]);
        QVector3D dir1(direction[1][0], direction[1][1], direction[1][2]);

        if (dir0.length() > 0.0f && dir1.length() > 0.0f) {
            QVector3D dir2 = QVector3D::crossProduct(dir1, dir0);

            if (dir2.length() > 0.0f) {
                dir0.normalize();
                dir1.normalize();
                dir2.normalize();

                rotation[0] = dir0.x(); rotation[1] = dir0.y(); rotation[2] = dir0.z();
                rotation[3] = dir1.x(); rotation[4] = dir1.y(); rotation[5] = dir1.z();
                rotation[6] = dir2.x(); rotation[7] = dir2.y(); rotation[8] = dir2.z();
            } else {
                rotation[0]=1; rotation[1]=0; rotation[2]=0;
                rotation[3]=0; rotation[4]=1; rotation[5]=0;
                rotation[6]=0; rotation[7]=0; rotation[8]=1;
            }
        } else {
            rotation[0]=1; rotation[1]=0; rotation[2]=0;
            rotation[3]=0; rotation[4]=1; rotation[5]=0;
            rotation[6]=0; rotation[7]=0; rotation[8]=1;
        }
    } else {
        rotation[0]=1; rotation[1]=0; rotation[2]=0;
        rotation[3]=0; rotation[4]=1; rotation[5]=0;
        rotation[6]=0; rotation[7]=0; rotation[8]=1;
    }

    if (coregl_ != nullptr) {
        coregl_->collectInstanceDataFromObjects(positionArray, solid, rotation, 1, false);
    }

    BOOST_FOREACH( globject &o, objects) {
        o.draw();
    }
}


glhalfcube * glhalfcube::do_clone()
{
    return new glhalfcube(*this);
}
