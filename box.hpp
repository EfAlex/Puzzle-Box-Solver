/*
    Copyright 2012 efalex <email>

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/


#ifndef BOX_HPP
#define BOX_HPP

#include "common.hpp"
#include <cstdint>
#include "cube.hpp"
#include "figure.hpp"

// Forward declaration — defined in box_solver.hpp
struct FigureRotation;

class box {
public:
    box();
    box(const box & other);
    box & operator=(const box & other);
    cube & getCube(const vector_int );
    cube & getCubeConst(const vector_int ) const;
    bool addFigure(figure &, const vector_int );
    bool delFigure(figure &, const vector_int );
    bool addFigureFromRotation(const FigureRotation &rot, const vector_int &v);
    bool delFigureFromRotation(const FigureRotation &rot, const vector_int &v);
    bool isEmpty() const;
    unsigned int getVolume() const;
    bool operator ==(const box &) const;
    bool operator !=(const box &) const;
    void dump();
    void rotateX();
    void rotateZ();
private:
    static constexpr int BOX_W = 4, BOX_H = 3, BOX_D = 2;
    static constexpr int BOX_CELLS = BOX_W * BOX_H * BOX_D; // 24

    // 4 bits per cell: 0=empty, 1=full, 2-13=12 diagonal directions
    // bits 3-2: zero-axis (00=x, 01=y, 10=z)
    // bits 1-0: signs of other two (+1=0, -1=1)
    uint8_t mask[12]; // 24 * 4 bits = 96 bits = 12 bytes

    mutable cube fullCube;

    inline uint8_t cellState(int x, int y, int z) const {
        int i = x + BOX_W * y + BOX_W * BOX_H * z;
        return (mask[i >> 1] >> ((i & 1) << 2)) & 0xf;
    }
    inline void setCellState(int x, int y, int z, uint8_t s) {
        int i = x + BOX_W * y + BOX_W * BOX_H * z;
        mask[i >> 1] &= (uint8_t)~(0xf << ((i & 1) << 2));
        mask[i >> 1] |= (uint8_t)(s << ((i & 1) << 2));
    }
    static inline uint8_t oppositeDir(uint8_t code) {
        // Lookup table for opposite direction codes
        // Pairs: 2<->5, 3<->4, 6<->7, 8<->11, 9<->10, 12<->13
        static const uint8_t opp[14] = {0,1,5,4,3,2,7,6,11,10,9,8,13,12};
        return (code >= 2 && code <= 13) ? opp[code] : code;
    }

    // Direction encoding: 4 bits (see decodeDir inverse)
    static uint8_t encodeDir(int x, int y, int z);
    static void decodeDir(uint8_t code, int &x, int &y, int &z);

    // Direction swap lookup for rotations
    // directionSwapZ[code] = new code after Z-rotation cell swap
    // directionSwapX[code] = new code after X-rotation cell swap
    static const uint8_t directionSwapZ[16];
    static const uint8_t directionSwapX[16];
};

#endif // BOX_HPP
