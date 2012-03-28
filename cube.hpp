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


#ifndef CUBE_HPP
#define CUBE_HPP

#include "common.hpp"

enum cell_t {
    cell_empty,
    cell_full,
    cell_half
};

class cube {
public:
    cube();
    cube(const cube &);
    cube &operator=(const cube &);
    bool isEmpty() const;
    bool isHalf() const;
    bool isFull() const;
    cell_t getState() const;
    int *getVector();
    void setState(const cell_t);
    bool setVector(const int *);
    void reset();
    bool operator ==(const cube&) const;
    bool operator !=(const cube&) const;
private:
    cell_t storage;
    int v[3];
};

#endif // CUBE_HPP
