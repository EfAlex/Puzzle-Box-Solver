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


#include "box.hpp"
#include "box_solver.hpp"
#include <boost/foreach.hpp>
#include <cstring>
#include <iostream>

// Decode lookup: direction code -> {x, y, z}
static const int dirVec[14][3] = {
    {0,0,0},   // 0: empty (unused)
    {0,0,0},   // 1: full (unused)
    {1,0,1},   // 2: +x +z (y=0)
    {1,0,-1},  // 3: +x -z (y=0)
    {-1,0,1},  // 4: -x +z (y=0)
    {-1,0,-1}, // 5: -x -z (y=0)
    {0,1,1},   // 6: +y +z (x=0)
    {0,-1,-1}, // 7: -y -z (x=0)
    {1,1,0},   // 8: +x +y (z=0)
    {1,-1,0},  // 9: +x -y (z=0)
    {-1,1,0},  // 10: -x +y (z=0)
    {-1,-1,0}, // 11: -x -y (z=0)
    {0,1,-1},  // 12: +y -z (x=0)
    {0,-1,1},  // 13: -y +z (x=0)
};

// Opposite direction lookup
static const uint8_t oppositeCode[14] = {
    0, 1, 5, 4, 3, 2, 7, 6, 11, 10, 9, 8, 13, 12
};

// Direction swap lookup for Z rotation: negate x,y
const uint8_t box::directionSwapZ[16] = {
    0, 1, 4, 5, 2, 3, 13, 12, 11, 10, 9, 8, 7, 6
};

// Direction swap lookup for X rotation: negate y,z
const uint8_t box::directionSwapX[16] = {
    0, 1, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12
};

box::box()
{
    std::memset(mask, 0, sizeof(mask));
    fullCube.setState(cell_full);
}

box::box(const box & other)
{
    std::memcpy(mask, other.mask, sizeof(mask));
    fullCube = other.fullCube;
}

box & box::operator=(const box & other)
{
    if (this != &other) {
        std::memcpy(mask, other.mask, sizeof(mask));
        fullCube = other.fullCube;
    }
    return *this;
}

cube & box::getCube(const vector_int v)
{
    fullCube.setState(cell_full);
    if (v.size() != 3) {
        return fullCube;
    }
    if (v[0] < 0 || v[0] >= BOX_W || v[1] < 0 || v[1] >= BOX_H || v[2] < 0 || v[2] >= BOX_D) {
        return fullCube;
    }
    uint8_t code = cellState(v[0], v[1], v[2]);
    fullCube.reset();
    if (code == 1) {
        fullCube.setState(cell_full);
    } else if (code >= 2 && code <= 13) {
        fullCube.setState(cell_half);
        fullCube.setVector(dirVec[code]);
    } else {
        fullCube.setState(cell_empty);
    }
    return fullCube;
}

cube & box::getCubeConst(const vector_int v) const
{
    fullCube.setState(cell_full);
    if (v.size() != 3) {
        return fullCube;
    }
    if (v[0] < 0 || v[0] >= BOX_W || v[1] < 0 || v[1] >= BOX_H || v[2] < 0 || v[2] >= BOX_D) {
        fullCube.setState(cell_full);
        return fullCube;
    }
    uint8_t code = cellState(v[0], v[1], v[2]);
    fullCube.reset();
    if (code == 1) {
        fullCube.setState(cell_full);
    } else if (code >= 2 && code <= 13) {
        fullCube.setState(cell_half);
        fullCube.setVector(dirVec[code]);
    } else {
        fullCube.setState(cell_empty);
    }
    return fullCube;
}

uint8_t box::encodeDir(int x, int y, int z)
{
    // Match against dirVec to find the code
    if (x == 1 && y == 0 && z == 1) return 2;
    if (x == 1 && y == 0 && z == -1) return 3;
    if (x == -1 && y == 0 && z == 1) return 4;
    if (x == -1 && y == 0 && z == -1) return 5;
    if (x == 0 && y == 1 && z == 1) return 6;
    if (x == 0 && y == -1 && z == -1) return 7;
    if (x == 1 && y == 1 && z == 0) return 8;
    if (x == 1 && y == -1 && z == 0) return 9;
    if (x == -1 && y == 1 && z == 0) return 10;
    if (x == -1 && y == -1 && z == 0) return 11;
    if (x == 0 && y == 1 && z == -1) return 12;
    if (x == 0 && y == -1 && z == 1) return 13;
    return 0;
}

void box::decodeDir(uint8_t code, int &x, int &y, int &z)
{
    if (code >= 2 && code <= 13) {
        x = dirVec[code][0];
        y = dirVec[code][1];
        z = dirVec[code][2];
    } else {
        x = y = z = 0;
    }
}

bool box::addFigureFromRotation(const FigureRotation &rot, const vector_int &v)
{
    // Phase 1: validate placement — must match original addFigure exactly
    for (int c = 0; c < rot.n_cubes; ++c) {
        int px = rot.pos[c][0] + v[0];
        int py = rot.pos[c][1] + v[1];
        int pz = rot.pos[c][2] + v[2];
        if (px < 0 || px >= 4 || py < 0 || py >= 3 || pz < 0 || pz >= 2) return false;
        uint8_t code = cellState(px, py, pz);
        if (rot.state[c] == cell_full && code != 0) return false;
        if (rot.state[c] == cell_half && code == 1) return false;
        if (rot.state[c] == cell_half && code >= 2 && code <= 13) {
            if (rot.dir[c][0] != -dirVec[code][0] ||
                rot.dir[c][1] != -dirVec[code][1] ||
                rot.dir[c][2] != -dirVec[code][2]) return false;
        }
    }
    // Phase 2: place cubes — must match original addFigure exactly
    for (int c = 0; c < rot.n_cubes; ++c) {
        int px = rot.pos[c][0] + v[0];
        int py = rot.pos[c][1] + v[1];
        int pz = rot.pos[c][2] + v[2];
        if (rot.state[c] == cell_full || (rot.state[c] == cell_half && cellState(px, py, pz) >= 2)) {
            setCellState(px, py, pz, 1);
        } else if (rot.state[c] == cell_half) {
            setCellState(px, py, pz, encodeDir(rot.dir[c][0], rot.dir[c][1], rot.dir[c][2]));
        }
    }
    // Phase 3: check half-cube adjacency — must match original addFigure exactly
    for (int c = 0; c < rot.n_cubes; ++c) {
        if (rot.state[c] != cell_half) continue;
        int px = rot.pos[c][0] + v[0];
        int py = rot.pos[c][1] + v[1];
        int pz = rot.pos[c][2] + v[2];
        if (cellState(px, py, pz) < 2) continue;
        int b = 0;
        for (int d = 0; d < 3; ++d) {
            if (rot.dir[c][d] != 0) {
                int nx = px, ny = py, nz = pz;
                if (d == 0) nx += rot.dir[c][0];
                else if (d == 1) ny += rot.dir[c][1];
                else nz += rot.dir[c][2];
                if (cellState(nx, ny, nz) == 1) b++;
            }
        }
        if (b >= 2) {
            delFigureFromRotation(rot, v);
            return false;
        }
    }
    return true;
}

bool box::delFigureFromRotation(const FigureRotation &rot, const vector_int &v)
{
    for (int c = 0; c < rot.n_cubes; ++c) {
        int px = rot.pos[c][0] + v[0];
        int py = rot.pos[c][1] + v[1];
        int pz = rot.pos[c][2] + v[2];
        uint8_t code = cellState(px, py, pz);
        if (rot.state[c] == cell_full && code == 1) {
            setCellState(px, py, pz, 0);
        } else if (rot.state[c] == cell_half && code >= 2) {
            setCellState(px, py, pz, 0);
        } else if (rot.state[c] == cell_half && code == 1) {
            uint8_t code2 = encodeDir(rot.dir[c][0], rot.dir[c][1], rot.dir[c][2]);
            setCellState(px, py, pz, oppositeCode[code2]);
        } else if (rot.state[c] == cell_half && code == 0) {
            // Cell was overwritten from code 1 to 0 during addFigure (half-cube on full cell)
            // Restore to code 1 to match original addFigure behavior
            setCellState(px, py, pz, 1);
        } else {
            return false;
        }
    }
    return true;
}

bool box::addFigure(figure & it, const vector_int v)
{
    for (uint8_t ci = 0; ci < it.n_cubes; ++ci) {
        figure_cube &c = it.cubes[ci];
        int px = c.pos[0] + v[0];
        int py = c.pos[1] + v[1];
        int pz = c.pos[2] + v[2];
        if (px < 0 || px >= BOX_W || py < 0 || py >= BOX_H || pz < 0 || pz >= BOX_D) {
            return false;
        }
        uint8_t code = cellState(px, py, pz);
        if (c.c.isFull() && code != 0) return false;
        if (c.c.isHalf() && code == 1) return false;
        if (c.c.isHalf() && code >= 2 && code <= 13) {
            const int *v1 = c.c.getVector();
            const int *v2 = dirVec[code];
            if (v1[0] != -v2[0] || v1[1] != -v2[1] || v1[2] != -v2[2]) {
                return false;
            }
        }
    }
    for (uint8_t ci = 0; ci < it.n_cubes; ++ci) {
        figure_cube &c = it.cubes[ci];
        int px = c.pos[0] + v[0];
        int py = c.pos[1] + v[1];
        int pz = c.pos[2] + v[2];
        if (c.c.isFull() || (c.c.isHalf() && cellState(px, py, pz) >= 2)) {
            setCellState(px, py, pz, 1);
        } else if (c.c.isHalf()) {
            setCellState(px, py, pz, encodeDir(c.c.getVector()[0],
                c.c.getVector()[1], c.c.getVector()[2]));
        }
    }
    for (uint8_t ci = 0; ci < it.n_cubes; ++ci) {
        figure_cube &c = it.cubes[ci];
        if (!c.c.isHalf()) continue;
        int px = c.pos[0] + v[0];
        int py = c.pos[1] + v[1];
        int pz = c.pos[2] + v[2];
        if (cellState(px, py, pz) < 2) continue;
        int b = 0;
        const int *dv = c.c.getVector();
        for (int d = 0; d < 3; ++d) {
            if (dv[d] != 0) {
                int nx = px, ny = py, nz = pz;
                if (d == 0) nx += dv[0];
                else if (d == 1) ny += dv[1];
                else nz += dv[2];
                if (cellState(nx, ny, nz) == 1) b++;
            }
        }
        if (b >= 2) {
            delFigure(it, v);
            return false;
        }
    }
    return true;
}

bool box::delFigure(figure & it, const vector_int v)
{
    for (uint8_t ci = 0; ci < it.n_cubes; ++ci) {
        figure_cube &c = it.cubes[ci];
        int px = c.pos[0] + v[0];
        int py = c.pos[1] + v[1];
        int pz = c.pos[2] + v[2];
        uint8_t code = cellState(px, py, pz);
        if (c.c.isFull() && code == 1) {
            setCellState(px, py, pz, 0);
        } else if (c.c.isHalf() && code >= 2) {
            setCellState(px, py, pz, 0);
        } else if (c.c.isHalf() && code == 1) {
            const int *orig = c.c.getVector();
            uint8_t code2 = encodeDir(orig[0], orig[1], orig[2]);
            setCellState(px, py, pz, oppositeCode[code2]);
        } else {
            return false;
        }
    }
    return true;
}

bool box::isEmpty() const
{
    for (int i = 0; i < 12; ++i) {
        if (mask[i] != 0) return false;
    }
    return true;
}

unsigned int box::getVolume() const
{
    unsigned int volume = 0;
    for (int i = 0; i < 12; ++i) {
        for (int bit = 0; bit < 4; ++bit) {
            uint8_t code = (mask[i] >> (bit << 2)) & 0xf;
            if (code == 1) volume += 2;
            else if (code >= 2) volume += 1;
        }
    }
    return volume;
}

bool box::operator ==(const box & other) const
{
    for (int i = 0; i < 12; ++i) {
        if (mask[i] != other.mask[i]) return false;
    }
    return true;
}

bool box::operator !=(const box & other) const
{
    return !(*this == other);
}

void box::dump()
{
    for (int z = 0; z < BOX_D; ++z) {
        for (int y = 0; y < BOX_H; ++y) {
            for (int x = 0; x < BOX_W; ++x) {
                uint8_t code = cellState(x, y, z);
                if (code == 1) {
                    std::cout << "X";
                } else if (code >= 2 && code <= 13) {
                    std::cout << "+" << dirVec[code][0]
                              << dirVec[code][1] << dirVec[code][2];
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
    // Z rotation: 180° in x-y plane: (x,y) -> (BOX_W-1-x, BOX_H-1-y) = (3-x, 2-y)
    // Creates 2-cycles (pairwise swap). Process each pair once via visited array.
    for (int z = 0; z < BOX_D; ++z) {
        uint8_t visited[BOX_W * BOX_H];
        std::memset(visited, 0, sizeof(visited));
        for (int by = 0; by < BOX_H; ++by) {
            for (int bx = 0; bx < BOX_W; ++bx) {
                if (visited[bx + by * BOX_W]) continue;
                int cx = bx, cy = by;
                // Trace the cycle (2-cycle for 180° rotation)
                int path[16], pathLen = 0;
                do {
                    path[pathLen++] = cx + cy * BOX_W;
                    visited[cx + cy * BOX_W] = 1;
                    int nx = BOX_W - 1 - cx;
                    int ny = BOX_H - 1 - cy;
                    cx = nx; cy = ny;
                } while (cx != bx || cy != by);
                // Swap values along the cycle
                uint8_t states[16], dirs[16];
                for (int i = 0; i < pathLen; ++i) {
                    int px = path[i] % BOX_W;
                    int py = path[i] / BOX_W;
                    states[i] = cellState(px, py, z) & 0x3;
                    dirs[i] = (cellState(px, py, z) >> 2) & 0xf;
                }
                for (int i = 0; i < pathLen; ++i) {
                    int dest = path[(i + 1) % pathLen];
                    int dx = dest % BOX_W;
                    int dy = dest / BOX_W;
                    setCellState(dx, dy, z, states[i]);
                    if (dirs[i] >= 2 && dirs[i] <= 13) {
                        setCellState(dx, dy, z,
                            (uint8_t)(states[i] | (directionSwapZ[dirs[i]] << 2)));
                    }
                }
            }
        }
    }
}

void box::rotateX()
{
    // X rotation: (y,z) -> (BOX_H-1-y, BOX_D-1-z) = (2-y, 1-z)
    // Must reset visited per x since cycles are per x-slice
    for (int x = 0; x < BOX_W; ++x) {
        uint8_t visited[BOX_H * BOX_D];
        std::memset(visited, 0, sizeof(visited));
        for (int by = 0; by < BOX_H; ++by) {
            for (int bz = 0; bz < BOX_D; ++bz) {
                if (visited[bz + by * BOX_D]) continue;
                int cy = by, cz = bz;
                int path[16], pathLen = 0;
                do {
                    path[pathLen++] = cy + cz * BOX_D;
                    visited[cy + cz * BOX_D] = 1;
                    int ny = BOX_H - 1 - cy;
                    int nz = BOX_D - 1 - cz;
                    cy = ny; cz = nz;
                } while (cy != by || cz != bz);
                uint8_t states[16], dirs[16];
                for (int i = 0; i < pathLen; ++i) {
                    int py = path[i] % BOX_H;
                    int pz = path[i] / BOX_H;
                    states[i] = cellState(x, py, pz) & 0x3;
                    dirs[i] = (cellState(x, py, pz) >> 2) & 0xf;
                }
                for (int i = 0; i < pathLen; ++i) {
                    int dest = path[(i + 1) % pathLen];
                    int dy = dest % BOX_H;
                    int dz = dest / BOX_H;
                    setCellState(x, dy, dz, states[i]);
                    if (dirs[i] >= 2 && dirs[i] <= 13) {
                        setCellState(x, dy, dz,
                            (uint8_t)(states[i] | (directionSwapX[dirs[i]] << 2)));
                    }
                }
            }
        }
    }
}
