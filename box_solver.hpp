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


#ifndef BOX_SOLVER_HPP
#define BOX_SOLVER_HPP

#include "common.hpp"
#include <boost/numeric/ublas/matrix.hpp>
#include "figure.hpp"
#include "box.hpp"
#include "box_solution.hpp"
#include <QThread>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <cstdint>
#include <cstring>

// Precomputed figure rotation — eliminates runtime rotate() allocations
struct FigureRotation {
    int pos[4][3];     // cube positions
    int dir[4][3];     // direction vectors (0,0,0 for full cubes)
    int dir0[3];       // rotated direction[0] (base 0,1,0 transformed by rotation)
    int dir1[3];       // rotated direction[1] (base 0,1,0 transformed by rotation)
    uint8_t state[4];  // cell_t values
    uint8_t n_cubes;   // actual cube count
};

class box_solver: public QThread {
    Q_OBJECT
public:
    const long check_count;

    box_solver();
    void run();
signals:
    void drawBox();
private slots:
    void stopComputing();
    void updateStatus();
private:
    int argc;
    char **argv;

    vector_int direction;
    std::vector < figure > figures, solution;
    std::vector < vector_int >solution_pos;
    std::vector < Mat3 > rotate_matrix;
    std::atomic<bool> stopComputingFlag;
    std::atomic<int> updateStatusFlag;  // 0=IDLE, 1=REQUESTED, 2=WRITING
    box b;                                // occupancy map — member like master

    /* Precomputed figure rotations — eliminates runtime rotate() + allocator */
    FigureRotation precomputed_rotations[8][24];  // 8 figures × 24 unique orientations
    uint8_t valid_rotation_count[8];             // unique orientations per figure
    void precomputeRotations();

    /* Dynamic thread feed */
    unsigned int spawnDepth;  // depth at which to spawn workers (default: 3)
    int maxThreads;           // = threadCount() = idealThreadCount() - 1
    std::atomic<int> activeThreads{0};  // protected by poolMutex for increments/decrements; atomic for reads
    std::mutex poolMutex;     // protects activeThreads + poolCv
    std::condition_variable poolCv;
    std::vector<std::thread> workerThreads;  // track all spawned threads
    std::mutex threadsMu;                    // protects workerThreads during join/clear
    std::atomic<bool> stopFlag;  // atomic version of stopComputingFlag

    /* Shared atomics: count + depth + solution_count — all threads access the same object */
    struct SharedState {
        std::atomic<long> count;
        std::atomic<long> depth[8];
        std::atomic<int> solution_count;  // atomic mirror of box_solution::solution_list.size()
    } shared;

    struct SubTask {
        unsigned int depth;
        std::vector<figure> sol;
        std::vector<vector_int> solPos;
        box b;            // carries occupancy map to worker
        std::vector<figure> figures;  // worker's own copy — avoids data race on parent's figures
        SharedState *shared;  // pointer to shared count/depth atomics
    };

    void initFigures();
    int solve();
    int iterateFigure(unsigned int);
    void init();
    void initSolution();
    void setRotateVector(std::array<std::array<int,3>,3> &m,
                         const std::array<int,3> &a,
                         const std::array<int,3> &b);
    void constructFigureFromRotation(unsigned int figIdx, int rotIdx, figure &out);

    void feedThread(SubTask &task);
    void workerRun(SubTask task);
    struct WorkerIterateState {
        box &b;
        std::vector<figure> &figures;
        const std::vector<Mat3> &rotateMatrix;
        std::vector<figure> &solution;
        std::vector<vector_int> &solution_pos;
        unsigned int &local_gl_pos;
        SharedState *shared;  // pointer to shared count/depth atomics
    };
    void workerIterate(unsigned int i, WorkerIterateState &s);
    int threadCount();
};

// Box symmetry — D2 group element
// Position transform: symPos[i] = sign[i] * p[i] + offset[i]
// Direction transform: symDir[i] = dsign[i] * dir[i]
struct BoxSymmetry {
    int sign[3];       // position sign
    int offset[3];     // position offset
    int dsign[3];      // direction sign
};

// D2 symmetry group for 4×3×2 box
static const BoxSymmetry boxSymmetries[4] = {
    // identity: (x, y, z) -> (x, y, z)
    { { 1,  1,  1}, {0, 0, 0}, { 1,  1,  1} },
    // rotateZ: 180° in xy plane: (x,y,z) -> (3-x, 2-y, z)
    { {-1, -1,  1}, {3, 2, 0}, {-1, -1,  1} },
    // rotateX: 180° in yz plane: (x,y,z) -> (x, 2-y, 1-z)
    { { 1, -1, -1}, {0, 2, 1}, { 1, -1, -1} },
    // rotateZX: compose rotateZ + rotateX: (x,y,z) -> (3-x, y, 1-z)
    { {-1,  1, -1}, {3, 0, 1}, {-1,  1, -1} },
};

#endif // BOX_SOLVER_HPP
