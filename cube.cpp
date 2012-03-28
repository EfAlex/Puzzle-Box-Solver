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


#include "cube.hpp"
#include <iostream>
#include <boost/numeric/ublas/io.hpp>

using namespace boost::numeric::ublas;

cube::cube():
        storage(cell_empty)
{
    v[0] = v[1] = v[2] = 0;
}

cube::cube(const cube &other)
{
    //std::cout << "Cube copy" << std::endl;
    storage = other.storage;
    v[0] = other.v[0];
    v[1] = other.v[1];
    v[2] = other.v[2];
}

cube & cube::operator=(const cube &other)
{
    if (this == &other) {
        return *this;
    }
    //std::cout << "Cube operator =" << std::endl;
    storage = other.storage;
    v[0] = other.v[0];
    v[1] = other.v[1];
    v[2] = other.v[2];
    return *this;
}

cell_t cube::getState(void) const
{
    return storage;
}

void cube::setState(const cell_t s)
{
    storage = s;
}

int *cube::getVector()
{
    return v;
}

bool cube::setVector(const int * nv)
{
    v[0] = nv[0];
    v[1] = nv[1];
    v[2] = nv[2];
    return true;
}

bool cube::isEmpty() const
{
    return storage == cell_empty;
}

bool cube::isHalf() const
{
    return storage == cell_half;
}

bool cube::isFull() const
{
    return storage == cell_full;
}

void cube::reset()
{
    storage = cell_empty;
    v[0] = v[1] = v[2] = 0;
}

bool cube::operator ==(const cube &other) const
{
    if (isEmpty() && other.isEmpty()) {
        return true;
    }
    if (isFull() && other.isFull()) {
        return true;
    }
    if (isHalf() && other.isHalf()) {
        return v[0] == other.v[0] && v[1] == other.v[1] && v[2] == other.v[2];
    }
    return false;
}
bool cube::operator !=(const cube &other) const
{
    return !(*this == other);
}
