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


#include "box.hpp"
#include <boost/foreach.hpp>
#include <boost/numeric/ublas/io.hpp>

using namespace boost::numeric::ublas;
box::box()
{
    content.resize(4);
    BOOST_FOREACH( matrix < cube >&x, content) {
        x.resize(3,2);
        for (int y = 0; y < 3 ; ++y) {
            for (int z =0 ; z < 2 ; ++z) {
                x(y,z).reset();
            }
        }
    }
}

cube & box::getCube(const vector_int v)
{
    fullCube.setState(cell_full);
    if (v.size() != 3) {
        return fullCube;
    }
    if (v[0] < 0 || v[0] >= 4 || v[1] < 0 || v[1] >= 3 || v[2] < 0 || v[2] >= 2) {
        return fullCube;
    }
    return content[v[0]](v[1],v[2]);
}

bool box::addFigure(figure & it, const vector_int v)
{
    // check ranges for box
    BOOST_FOREACH(figure_cube & c, it.cubes) {
        vector_int p(3);
        p[0] = c.pos[0] + v[0];
        p[1] = c.pos[1] + v[1];
        p[2] = c.pos[2] + v[2];
        if (p[0] < 0 || p[0] >= 4 || p[1] < 0 || p[1] >= 3 || p[2] < 0 || p[2] >= 2) {
            return false;
        }
        if ((c.c.isFull() && !getCube(p).isEmpty()) || (c.c.isHalf() && getCube(p).isFull())) {
            return false;
        }
        if (c.c.isHalf() && getCube(p).isHalf()) {
            int *v1 = c.c.getVector(), *v2 = getCube(p).getVector();
            if (v1[0] != -v2[0] || v1[1] != -v2[1] || v1[2] != -v2[2]) {
                return false;
            }
        }
    }
    // all is OK
    // insert figure into box

    BOOST_FOREACH(figure_cube & c, it.cubes) {
        vector_int p(3);
        p[0] = c.pos[0] + v[0];
        p[1] = c.pos[1] + v[1];
        p[2] = c.pos[2] + v[2];
        if ((c.c.isFull() && getCube(p).isEmpty())
                || (c.c.isHalf() && getCube(p).isHalf())) {
            getCube(p).setState(cell_full);
        } else if (c.c.isHalf() && getCube(p).isEmpty()) {
            getCube(p).setState(cell_half);
            getCube(p).setVector(c.c.getVector());
        } else {
            std:: cerr << __FILE__ << ":" << __LINE__ << " Insert error!" << std::endl;
            exit(1);
            return false;
        }
    }
    // Do other checks
#if 1
    BOOST_FOREACH(figure_cube & c, it.cubes) {
        vector_int p(3);
        p[0] = c.pos[0] + v[0];
        p[1] = c.pos[1] + v[1];
        p[2] = c.pos[2] + v[2];
        if (c.c.isHalf() && getCube(p).isHalf()) {
            // check for dead combinations
            int b = 0;
            for (int d = 0; d < 3; ++d) {
                if (c.c.getVector()[d] != 0) {
                    vector_int near(3);
                    near[0] = near[1] = near[2] = 0;
                    near[d] = c.c.getVector()[d];
                    near += p;
                    if (getCube(near).isFull()) {
                        b++;
                    }
                }
            }
            if (b >= 2) {
                // Not possible to insert other figures
                // Restore origianl state
                delFigure(it, v);
                return false;
            }
        }
    }
#endif
    return true;
}

bool box::delFigure(figure & it, const vector_int v)
{
    BOOST_FOREACH(figure_cube & c, it.cubes) {
        vector_int p(3);
        p[0] = c.pos[0] + v[0];
        p[1] = c.pos[1] + v[1];
        p[2] = c.pos[2] + v[2];
        if ((c.c.isFull() && getCube(p).isFull()) || (c.c.isHalf() && getCube(p).isHalf())) {
            getCube(p).setState(cell_empty);
            int nv[3] = {0, 0, 0};
            getCube(p).setVector(nv);
        } else if (c.c.isHalf() && getCube(p).isFull()) {
            getCube(p).setState(cell_half);
            int nv[3];
            int *orig;
            orig = c.c.getVector();
            nv[0] = -orig[0];
            nv[1] = -orig[1];
            nv[2] = -orig[2];
            getCube(p).setVector(nv);
        } else {
            std::cerr << __FILE__ << ":" << __LINE__ << " Remove error!" << std::endl;
            throw new std::exception();
            exit(1);
            return false;
        }
    }
    return true;
}

bool box::isEmpty() const
{
    BOOST_FOREACH(const matrix < cube >&x, content) {
        for (int y = 0; y < 3 ; ++y) {
            for (int z =0 ; z < 2 ; ++z) {
                if (!x(y,z).isEmpty()) {
                    return false;
                }
            }
        }
    }
    return true;
}

unsigned int box::getVolume() const
{
    unsigned int volume = 0;
    BOOST_FOREACH(const matrix < cube >&x, content) {
        for (int y = 0; y < 3 ; ++y) {
            for (int z =0 ; z < 2 ; ++z) {
                switch (x(y,z).getState()) {
                    case cell_half:
                        volume++;
                        break;
                    case cell_full:
                        volume += 2;
                        break;
                    default:
                        break;
                }
            }
        }
    }
    return volume;
}

bool box::operator ==(const box & other) const
{
    for (int x = 0 ; x < 4; ++x) {
        for (int y = 0; y < 3; ++y) {
            for (int z = 0; z < 2; ++z) {
                if(content[x](y,z) != other.content[x](y,z)) {
                    return false;
                }
            }
        }
    }
    return true;
}

bool box::operator !=(const box & other) const
{
    return !(*this == other);
}

#if 0
box & box::operator =(const box & other)
{
    if (this != &other) {
        for (int x = 0 ; x < 4; ++x) {
            for (int y = 0; y < 3; ++y) {
                for (int z = 0; z < 2; ++z) {
                    content[x](y,z) = other.content[x](y,z);
                }
            }
        }
    }
    return *this;
}
#endif

void box::dump()
{
    vector_int v(3);
    for(int z = 0; z < 2; ++z) {
        v[2] = z;
        for (int y = 0; y < 3; ++y) {
            v[1] = y;
            for (int x=0; x < 4; ++x) {
                cube c;
                v[0] = x;
                c = getCube(v);
                if (c.isFull()) {
                    std::cout << "X";
                } else if(c.isHalf()) {
                    int *orig = c.getVector();
                    vector_int v (3);
                    v[0] = orig[0];
                    v[1] = orig[1];
                    v[2] = orig[2];
                    std::cout << "+" << v;
                } else {
                    std::cout << ".";
                }
            }
            std::cout << ' ';
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

void box::rotateZ()
{
    vector_int p1(3), p2(3);
    for(int z = 0; z < 2; ++z) {
        p1[2] = z;
        p2[2] = z;
        for (int y = 0; y < 3; ++y) {
            p1[1] = y;
            p2[1] = 2 - y;
            for (int x=0; x < 2; ++x) {
                p1[0] = x;
                p2[0] = 3 - x;
                cube c1, c2;
                c1 = getCube(p1);
                c2 = getCube(p2);
                int *v1 = c1.getVector(), *v2 = c2.getVector();
                v1[0] = -v1[0]; v1[1] = -v1[1];
                v2[0] = -v2[0]; v2[1] = -v2[1];
                getCube(p1).setState(c2.getState());
                getCube(p1).setVector(v2);
                getCube(p2).setState(c1.getState());
                getCube(p2).setVector(v1);
            }
        }
    }
}
void box::rotateX()
{
    vector_int p1(3), p2(3);
    for(int z = 0; z < 1; ++z) {
        p1[2] = z;
        p2[2] = 1 - z;
        for (int y = 0; y < 3; ++y) {
            p1[1] = y;
            p2[1] = 2 - y;
            for (int x=0; x < 4; ++x) {
                p1[0] = x;
                p2[0] = x;
                cube c1, c2;
                c1 = getCube(p1);
                c2 = getCube(p2);
                int *v1 = c1.getVector(), *v2 = c2.getVector();
                v1[1] = -v1[1]; v1[2] = -v1[2];
                v2[1] = -v2[1]; v2[2] = -v2[2];
                getCube(p1).setState(c2.getState());
                getCube(p1).setVector(v2);
                getCube(p2).setState(c1.getState());
                getCube(p2).setVector(v1);
            }
        }
    }
}
