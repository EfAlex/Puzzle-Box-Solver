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


#ifndef BOX_HPP
#define BOX_HPP

#include "common.hpp"
#include "cube.hpp"
#include "figure.hpp"

class box {
public:
    box();
    cube & getCube(const vector_int );
    cube & getCubeConst(const vector_int ) const;
    bool addFigure(figure &, const vector_int );
    bool delFigure(figure &, const vector_int );
    bool isEmpty() const;
    unsigned int getVolume() const;
    bool operator ==(const box &) const;
    bool operator !=(const box &) const;
    void dump();
//    box & operator =(const box&);
    void rotateX();
    void rotateZ();
private:
    boost::numeric::ublas::vector < boost::numeric::ublas::matrix < cube > > content;
    cube fullCube;
};

#endif // BOX_HPP
