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


#include "box_solver.hpp"
#include <algorithm>
#include <limits.h>
#include <boost/foreach.hpp>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <iostream>
#include "box.hpp"
#include "figure.hpp"
#include "box_solution.hpp"
#include "glwidget.hpp"
#include <chrono>

// Return "HH:MM:SS,uuuuuu" with fixed width and leading zeros
inline std::string mt_ts() {
    auto now = std::chrono::system_clock::now();
    auto ms  = std::chrono::duration_cast<std::chrono::microseconds>(
                   now.time_since_epoch()) % 1000000;
    auto tp  = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
    localtime_r(&tp, &tm);
    char buf[24];
    std::snprintf(buf, sizeof(buf), "%02d:%02d:%02d,%06ld",
                  tm.tm_hour, tm.tm_min, tm.tm_sec, ms.count());
    return std::string(buf);
}

int box_solver::threadCount()
{
    int n = QThread::idealThreadCount();
    return n <= 2 ? n : n - 1;
}

void box_solver::init()
{
    shared.count.store(0, std::memory_order_relaxed);
    rotate_matrix.resize(0);
    Mat3 r;
    r[0] = {{ 1,  0,  0}}; r[1] = {{ 0,  0, -1}}; r[2] = {{ 0,  1,  0}};
    rotate_matrix.push_back(r);
    r[0] = {{ 0,  0,  1}}; r[1] = {{ 0,  1,  0}}; r[2] = {{-1,  0,  0}};
    rotate_matrix.push_back(r);
    r[0] = {{ 0, -1,  0}}; r[1] = {{ 1,  0,  0}}; r[2] = {{ 0,  0,  1}};
    rotate_matrix.push_back(r);
}
box_solver::box_solver():check_count(1000000),stopComputingFlag(false),updateStatusFlag(0),spawnDepth(0),maxThreads(0),activeThreads(0),stopFlag(false)
{
    init();
}

void box_solver::run()
{
    initFigures();
    precomputeRotations();
    spawnDepth = 1;
    maxThreads = threadCount();
    activeThreads.store(0, std::memory_order_relaxed);
    // std::cerr << "[" << mt_ts() << "] [MT] idealThreadCount=" << QThread::idealThreadCount() << " maxThreads=" << maxThreads << " spawnDepth=" << spawnDepth << " figures.size()=" << figures.size() << std::endl;
    solve();
}

int box_solver::solve()
{
    activeThreads.store(0, std::memory_order_relaxed);
    stopFlag.store(false, std::memory_order_relaxed);
    shared.count.store(0, std::memory_order_relaxed);
    for (int d = 0; d < 8; ++d) shared.depth[d].store(0, std::memory_order_relaxed);
    std::cout << "[MT] check_count=" << check_count << std::endl;
    workerThreads.clear();

    int res = iterateFigure(0);  // master code, spawns workers at spawnDepth

    {
        std::unique_lock<std::mutex> lock(poolMutex);
        poolCv.wait(lock, [this]() { return activeThreads == 0; });
    }
    { std::lock_guard<std::mutex> lk(threadsMu);
      for (auto &th : workerThreads) { if (th.joinable()) th.join(); }
      workerThreads.clear(); }

    long total_iterations = shared.count.load(std::memory_order_relaxed);
    size_t total_solutions = box_solution::solution_list_size();
    std::cout << "=== Search complete ===" << std::endl;
    std::cout << "Total iterations: " << total_iterations << std::endl;
    std::cout << "Total solutions:  " << total_solutions << std::endl;
    std::cout << "=====================" << std::endl;

    if (!box_solution::empty()) {
        GLWidget::gl_pos = box_solution::solution_list_size() - 1;
        box_solution::solution_list[GLWidget::gl_pos].to_vector(
            GLWidget::gl_solution, GLWidget::gl_solution_pos);
        GLWidget::gl_count = GLWidget::gl_solution.size();
    }
    Q_EMIT box_solver::drawBox();
    return res;
}

int box_solver::iterateFigure(unsigned int i)
{
    if (stopFlag.load(std::memory_order_relaxed)) return 0;
    int fig = i;
    for (int r = 0; r < valid_rotation_count[fig]; ++r) {
        const auto &rot = precomputed_rotations[fig][r];
        // find figure boundaries from precomputed data (no ublas allocs)
        int min_x = INT_MAX, max_x = INT_MIN, min_y = INT_MAX, max_y = INT_MIN, min_z = INT_MAX, max_z = INT_MIN;
        for (int c = 0; c < rot.n_cubes; ++c) {
            if (rot.pos[c][0] < min_x) min_x = rot.pos[c][0];
            if (rot.pos[c][0] > max_x) max_x = rot.pos[c][0];
            if (rot.pos[c][1] < min_y) min_y = rot.pos[c][1];
            if (rot.pos[c][1] > max_y) max_y = rot.pos[c][1];
            if (rot.pos[c][2] < min_z) min_z = rot.pos[c][2];
            if (rot.pos[c][2] > max_z) max_z = rot.pos[c][2];
        }
        if (max_x - min_x > 4 || max_y - min_y > 3 || max_z - min_z > 2) continue;
        // Move figure in the box
        for (int x = -min_x; x < 4 - max_x; ++x) {
            for (int y = -min_y; y < 3 - max_y; ++y) {
                for (int z = -min_z; z < 2 - max_z; ++z) {
                    vector_int v(3);
                    v[0] = x; v[1] = y; v[2] = z;
                    if (b.addFigureFromRotation(rot, v)) {
                        int count = this->shared.count.fetch_add(1, std::memory_order_relaxed) + 1;
                        this->shared.depth[i].fetch_add(1, std::memory_order_relaxed);
                        // Construct rotated figure from precomputed data for solution storage
                        constructFigureFromRotation(fig, r, solution[i]);
                        solution_pos[i] = v;

                        bool claimed = false;
                        if (count % check_count == 0) {
                            updateStatusFlag.store(2, std::memory_order_relaxed);
                            claimed = true;
                            std::cout << "[MT] check_count fired: count=" << count << " check_count=" << check_count << std::endl;
                        } else if (updateStatusFlag.load(std::memory_order_relaxed) == 1) {
                            int expected = 1;
                            claimed = updateStatusFlag.compare_exchange_strong(expected, 2, std::memory_order_relaxed);
                        }
                        if (claimed) {
                            GLWidget::setGLSolution(i + 1, solution, solution_pos);
                            std::cout << "Iteration: " << count << std::endl;
                            for (unsigned int si = 0; si <= i; ++si) {
                                std::cout << "Item: " << si
                                    << " Position: ("
                                    << solution_pos[si](0) << ","
                                    << solution_pos[si](1) << ","
                                    << solution_pos[si](2) << ")"
                                    << " Direction[0]: ("
                                    << solution[si].direction[0](0) << ","
                                    << solution[si].direction[0](1) << ","
                                    << solution[si].direction[0](2) << ")"
                                    << " Direction[1]: ("
                                    << solution[si].direction[1](0) << ","
                                    << solution[si].direction[1](1) << ","
                                    << solution[si].direction[1](2) << ")"
                                    << std::endl;
                            }
                            std::cout << "Depth: [";
                            for (unsigned int d = 0; d < 8; ++d)
                                std::cout << shared.depth[d].load(std::memory_order_relaxed)
                                    << (d + 1 < 8 ? "," : "");
                            std::cout << "]" << std::endl;
                            std::cout << std::endl;
                            Q_EMIT box_solver::drawBox();
                            updateStatusFlag.store(0, std::memory_order_relaxed);
                        }
                        if (i == spawnDepth) {
                            box b_snapshot = b;
                            SubTask task;
                            task.b = b_snapshot;
                            task.depth = i + 1;
                            task.sol = solution;
                            task.solPos = solution_pos;
                            task.figures = figures;
                            task.shared = &this->shared;
                            feedThread(task);
                        } else {
                            iterateFigure(i + 1);
                        }
                        if (!b.delFigureFromRotation(rot, v)) {
                            std::cerr << "Delete error!" << std::endl;
                            exit(1);
                        }
                        if (stopFlag.load(std::memory_order_acquire)) return 0;
                    }
                }
            }
        }
    }
    return 0;
}

void box_solver::feedThread(SubTask &task)
{
    std::unique_lock<std::mutex> lock(poolMutex);
    while (!stopFlag.load(std::memory_order_relaxed) && activeThreads >= maxThreads) {
        // std::cerr << "[" << mt_ts() << "] [MT] feedThread blocked: active=" << activeThreads << " max=" << maxThreads << std::endl;
        poolCv.wait(lock, [this]() { return activeThreads < maxThreads || stopFlag.load(std::memory_order_acquire); });
    }
    if (stopFlag.load(std::memory_order_relaxed)) return;
    activeThreads.fetch_add(1, std::memory_order_relaxed);
    lock.unlock();
    // std::cerr << "[" << mt_ts() << "] [MT] feedThread launched: active=" << activeThreads << std::endl;
    workerThreads.emplace_back([this, task]() {
        // std::cerr << "[" << mt_ts() << "] [MT] workerRun started" << std::endl;
        workerRun(task);
        { std::lock_guard<std::mutex> lock(poolMutex); activeThreads.fetch_sub(1, std::memory_order_release); }
        poolCv.notify_all();
    });
}

void box_solver::workerRun(SubTask task)
{
    box b;
    std::vector<figure> solution(task.sol);
    std::vector<vector_int> solution_pos(task.solPos);

    b = task.b;  // restore parent's occupancy map

    WorkerIterateState s{b, task.figures, rotate_matrix, solution, solution_pos,
                         GLWidget::gl_pos, task.shared};
    workerIterate(task.depth, s);
}

void box_solver::workerIterate(unsigned int i, WorkerIterateState &s)
{
    box &b = s.b;
    auto &figures = s.figures;
    auto &solution = s.solution;
    auto &solution_pos = s.solution_pos;
    auto &local_gl_pos = s.local_gl_pos;
    if (stopFlag.load(std::memory_order_acquire)) return;
    if (i >= figures.size()) {
        if (box_solution::isUnique(solution, solution_pos)) {
            box_solution::addSolution(solution, solution_pos);
            GLWidget::setGLSolution(figures.size(), solution, solution_pos);
            GLWidget::gl_pos = box_solution::solution_list_size() - 1;
            std::cout << "Solution found!" << std::endl << " After " << s.shared->count << " iterations" << std::endl;
            std::cout << "Solution: " << local_gl_pos << std::endl;
            std::cout << box_solution(solution, solution_pos);
            std::cout << "Depth: [";
            for (unsigned int d = 0; d < 8; ++d)
                std::cout << s.shared->depth[d].load(std::memory_order_relaxed)
                          << (d + 1 < 8 ? "," : "");
            std::cout << "]" << std::endl;
            std::cout << std::endl;
            Q_EMIT box_solver::drawBox();
        }
        return;
    }
    // Use precomputed rotations — no runtime rotate() calls
    for (int r = 0; r < valid_rotation_count[i]; ++r) {
        const auto &rot = precomputed_rotations[i][r];
        // find figure boundaries from precomputed data
        int min_x = INT_MAX, max_x = INT_MIN, min_y = INT_MAX, max_y = INT_MIN, min_z = INT_MAX, max_z = INT_MIN;
        for (int c = 0; c < rot.n_cubes; ++c) {
            if (rot.pos[c][0] < min_x) min_x = rot.pos[c][0];
            if (rot.pos[c][0] > max_x) max_x = rot.pos[c][0];
            if (rot.pos[c][1] < min_y) min_y = rot.pos[c][1];
            if (rot.pos[c][1] > max_y) max_y = rot.pos[c][1];
            if (rot.pos[c][2] < min_z) min_z = rot.pos[c][2];
            if (rot.pos[c][2] > max_z) max_z = rot.pos[c][2];
        }
        if (max_x - min_x > 4 || max_y - min_y > 3 || max_z - min_z > 2) continue;
        for (int x = -min_x; x < 4 - max_x; ++x) {
            for (int y = -min_y; y < 3 - max_y; ++y) {
                for (int z = -min_z; z < 2 - max_z; ++z) {
                    vector_int v(3);
                    v[0] = x; v[1] = y; v[2] = z;
                    if (b.addFigureFromRotation(rot, v)) {
                        int count = s.shared->count.fetch_add(1, std::memory_order_relaxed) + 1;
                        s.shared->depth[i].fetch_add(1, std::memory_order_relaxed);
                        constructFigureFromRotation(i, r, solution[i]);
                        solution_pos[i] = v;

                        bool claimed = false;
                        if (count % check_count == 0) {
                            updateStatusFlag.store(2, std::memory_order_relaxed);
                            claimed = true;
                            std::cout << "[MT] check_count fired: count=" << count << " check_count=" << check_count << std::endl;
                        } else if (updateStatusFlag.load(std::memory_order_relaxed) == 1) {
                            int expected = 1;
                            claimed = updateStatusFlag.compare_exchange_strong(expected, 2, std::memory_order_relaxed);
                        }
                        if (claimed) {
                            GLWidget::setGLSolution(i + 1, solution, solution_pos);
                            std::cout << "Iteration: " << count << std::endl;
                            for (unsigned int si = 0; si <= i; ++si) {
                                std::cout << "Item: " << si
                                    << " Position: ("
                                    << solution_pos[si](0) << ","
                                    << solution_pos[si](1) << ","
                                    << solution_pos[si](2) << ")"
                                    << " Direction[0]: ("
                                    << solution[si].direction[0](0) << ","
                                    << solution[si].direction[0](1) << ","
                                    << solution[si].direction[0](2) << ")"
                                    << " Direction[1]: ("
                                    << solution[si].direction[1](0) << ","
                                    << solution[si].direction[1](1) << ","
                                    << solution[si].direction[1](2) << ")"
                                    << std::endl;
                            }
                            std::cout << "Depth: [";
                            for (unsigned int d = 0; d < 8; ++d)
                                std::cout << shared.depth[d].load(std::memory_order_relaxed)
                                    << (d + 1 < 8 ? "," : "");
                            std::cout << "]" << std::endl;
                            std::cout << std::endl;
                            Q_EMIT box_solver::drawBox();
                            updateStatusFlag.store(0, std::memory_order_relaxed);
                        }
                        workerIterate(i + 1, s);
                        if (!b.delFigureFromRotation(rot, v)) {
                            std::cerr << "Delete error!" << std::endl;
                            exit(1);
                        }
                        if (stopFlag.load(std::memory_order_acquire)) return;
                    }
                }
            }
        }
    }
}

void box_solver::initFigures()
{
    figures.resize(0);
    solution.resize(8);
    solution_pos.resize(8);
    for (unsigned int p = 0; p < solution_pos.size(); ++p) {
        solution_pos[p].resize(3);
    }
    // Figure 1
    {
        figure f;
        {
            figure_cube fc;
            fc.c.setState(cell_full);
            fc.pos[0] = 0;
            fc.pos[1] = 0;
            fc.pos[2] = 0;
            f.addFigureCube(fc);
        }
        {
            figure_cube fc;
            fc.c.setState(cell_full);
            fc.pos[0] = 1;
            fc.pos[1] = 0;
            fc.pos[2] = 0;
            f.addFigureCube(fc);
        }
        {
            figure_cube fc;
            fc.c.setState(cell_half);
            fc.pos[0] = 0;
            fc.pos[1] = 0;
            fc.pos[2] = 1;
            int v[3];
            v[0] = 0;
            v[1] = -1;
            v[2] = 1;
            fc.c.setVector(v);
            f.addFigureCube(fc);
        }
        {
            figure_cube fc;
            fc.c.setState(cell_half);
            fc.pos[0] = 2;
            fc.pos[1] = 0;
            fc.pos[2] = 0;
            int v[3];
            v[0] = 1;
            v[1] = 0;
            v[2] = 1;
            fc.c.setVector(v);
            f.addFigureCube(fc);
        }
        figures.push_back(f);
    }
    // Figure 2
    {
        figure f;
        {
            figure_cube fc;
            fc.c.setState(cell_full);
            fc.pos[0] = 0;
            fc.pos[1] = 0;
            fc.pos[2] = 0;
            f.addFigureCube(fc);
        }
        {
            figure_cube fc;
            fc.c.setState(cell_full);
            fc.pos[0] = 1;
            fc.pos[1] = 0;
            fc.pos[2] = 0;
            f.addFigureCube(fc);
        }
        {
            figure_cube fc;
            fc.c.setState(cell_half);
            fc.pos[0] = 0;
            fc.pos[1] = -1;
            fc.pos[2] = 0;
            int v[3];
            v[0] = -1;
            v[1] = -1;
            v[2] = 0;
            fc.c.setVector(v);
            f.addFigureCube(fc);
        }
        {
            figure_cube fc;
            fc.c.setState(cell_half);
            fc.pos[0] = 1;
            fc.pos[1] = -1;
            fc.pos[2] = 0;
            int v[3];
            v[0] = 0;
            v[1] = -1;
            v[2] = 1;
            fc.c.setVector(v);
            f.addFigureCube(fc);
        }
        figures.push_back(f);
    }
    // Figure 3
    {
        figure f;
        {
            figure_cube fc;
            fc.c.setState(cell_full);
            fc.pos[0] = 0;
            fc.pos[1] = 0;
            fc.pos[2] = 0;
            f.addFigureCube(fc);
        }
        {
            figure_cube fc;
            fc.c.setState(cell_full);
            fc.pos[0] = 1;
            fc.pos[1] = 0;
            fc.pos[2] = 0;
            f.addFigureCube(fc);
        }
        {
            figure_cube fc;
            fc.c.setState(cell_half);
            fc.pos[0] = 0;
            fc.pos[1] = -1;
            fc.pos[2] = 0;
            int v[3];
            v[0] = 1;
            v[1] = -1;
            v[2] = 0;
            fc.c.setVector(v);
            f.addFigureCube(fc);
        }
        {
            figure_cube fc;
            fc.c.setState(cell_half);
            fc.pos[0] = 2;
            fc.pos[1] = 0;
            fc.pos[2] = 0;
            int v[3];
            v[0] = 1;
            v[1] = 0;
            v[2] = 1;
            fc.c.setVector(v);
            f.addFigureCube(fc);
        }
        figures.push_back(f);
    }
    // Figure 4
    {
        figure f;
        {
            figure_cube fc;
            fc.c.setState(cell_full);
            fc.pos[0] = 0;
            fc.pos[1] = 0;
            fc.pos[2] = 0;
            f.addFigureCube(fc);
        }
        {
            figure_cube fc;
            fc.c.setState(cell_full);
            fc.pos[0] = 1;
            fc.pos[1] = 0;
            fc.pos[2] = 0;
            f.addFigureCube(fc);
        }
        {
            figure_cube fc;
            fc.c.setState(cell_half);
            fc.pos[0] = 0;
            fc.pos[1] = 1;
            fc.pos[2] = 0;
            int v[3];
            v[0] = 0;
            v[1] = 1;
            v[2] = 1;
            fc.c.setVector(v);
            f.addFigureCube(fc);
        }
        {
            figure_cube fc;
            fc.c.setState(cell_half);
            fc.pos[0] = 1;
            fc.pos[1] = -1;
            fc.pos[2] = 0;
            int v[3];
            v[0] = -1;
            v[1] = -1;
            v[2] = 0;
            fc.c.setVector(v);
            f.addFigureCube(fc);
        }
        figures.push_back(f);
    }
    // Figure 5
    {
        figure f;
        {
            figure_cube fc;
            fc.c.setState(cell_full);
            fc.pos[0] = 0;
            fc.pos[1] = 0;
            fc.pos[2] = 0;
            f.addFigureCube(fc);
        }
        {
            figure_cube fc;
            fc.c.setState(cell_full);
            fc.pos[0] = 0;
            fc.pos[1] = -1;
            fc.pos[2] = 0;
            f.addFigureCube(fc);
        }
        {
            figure_cube fc;
            fc.c.setState(cell_half);
            fc.pos[0] = 0;
            fc.pos[1] = -1;
            fc.pos[2] = 1;
            int v[3];
            v[0] = 0;
            v[1] = 1;
            v[2] = 1;
            fc.c.setVector(v);
            f.addFigureCube(fc);
        }
        {
            figure_cube fc;
            fc.c.setState(cell_half);
            fc.pos[0] = 1;
            fc.pos[1] = 0;
            fc.pos[2] = 0;
            int v[3];
            v[0] = 1;
            v[1] = -1;
            v[2] = 0;
            fc.c.setVector(v);
            f.addFigureCube(fc);
        }
        figures.push_back(f);
    }
    // Figure 6
    {
        figure f;
        {
            figure_cube fc;
            fc.c.setState(cell_full);
            fc.pos[0] = 0;
            fc.pos[1] = 0;
            fc.pos[2] = 0;
            f.addFigureCube(fc);
        }
        {
            figure_cube fc;
            fc.c.setState(cell_full);
            fc.pos[0] = 0;
            fc.pos[1] = 0;
            fc.pos[2] = 1;
            f.addFigureCube(fc);
        }
        {
            figure_cube fc;
            fc.c.setState(cell_half);
            fc.pos[0] = 1;
            fc.pos[1] = 0;
            fc.pos[2] = 0;
            int v[3];
            v[0] = 1;
            v[1] = 0;
            v[2] = 1;
            fc.c.setVector(v);
            f.addFigureCube(fc);
        }
        {
            figure_cube fc;
            fc.c.setState(cell_half);
            fc.pos[0] = 0;
            fc.pos[1] = -1;
            fc.pos[2] = 1;
            int v[3];
            v[0] = 1;
            v[1] = -1;
            v[2] = 0;
            fc.c.setVector(v);
            f.addFigureCube(fc);
        }
        figures.push_back(f);
    }
    //Figure 7
    {
        figure f;
        {
            figure_cube fc;
            fc.c.setState(cell_full);
            fc.pos[0] = 0;
            fc.pos[1] = -1;
            fc.pos[2] = 0;
            f.addFigureCube(fc);
        }
        {
            figure_cube fc;
            fc.c.setState(cell_full);
            fc.pos[0] = 1;
            fc.pos[1] = 0;
            fc.pos[2] = 0;
            f.addFigureCube(fc);
        }
        {
            figure_cube fc;
            fc.c.setState(cell_half);
            fc.pos[0] = 0;
            fc.pos[1] = 0;
            fc.pos[2] = 0;
            int v[3];
            v[0] = -1;
            v[1] = 1;
            v[2] = 0;
            fc.c.setVector(v);
            f.addFigureCube(fc);
        }
        {
            figure_cube fc;
            fc.c.setState(cell_half);
            fc.pos[0] = 1;
            fc.pos[1] = -1;
            fc.pos[2] = 0;
            int v[3];
            v[0] = 1;
            v[1] = 0;
            v[2] = 1;
            fc.c.setVector(v);
            f.addFigureCube(fc);
        }
        figures.push_back(f);
    }
    //Figure 8
    {
        figure f;
        {
            figure_cube fc;
            fc.c.setState(cell_full);
            fc.pos[0] = 1;
            fc.pos[1] = 0;
            fc.pos[2] = 0;
            f.addFigureCube(fc);
        }
        {
            figure_cube fc;
            fc.c.setState(cell_full);
            fc.pos[0] = 0;
            fc.pos[1] = 0;
            fc.pos[2] = 1;
            f.addFigureCube(fc);
        }
        {
            figure_cube fc;
            fc.c.setState(cell_half);
            fc.pos[0] = 0;
            fc.pos[1] = 0;
            fc.pos[2] = 0;
            int v[3];
            v[0] = 0;
            v[1] = -1;
            v[2] = -1;
            fc.c.setVector(v);
            f.addFigureCube(fc);
        }
        {
            figure_cube fc;
            fc.c.setState(cell_half);
            fc.pos[0] = 1;
            fc.pos[1] = 0;
            fc.pos[2] = 1;
            int v[3];
            v[0] = 0;
            v[1] = 1;
            v[2] = 1;
            fc.c.setVector(v);
            f.addFigureCube(fc);
        }
        figures.push_back(f);
    }
}

// Generate all 24 unique rotation matrices from the 3 basis rotations.
// The original code uses rotate_matrix[0] (X 90°), rotate_matrix[1] (Y 90°),
// rotate_matrix[2] (Z 90°) — these 3 quarter-turns generate all 24 cube orientations.
// We enumerate them directly: for each of 6 face directions as "forward",
// pick one of 4 remaining axes as "up" → 6 × 4 = 24 orientations.
static void generate24Rotations(Mat3 out[24]) {
    // Basis vectors: e0=(1,0,0), e1=(0,1,0), e2=(0,0,1)
    // For each target X axis (6 choices: ±e0, ±e1, ±e2)
    int axes[6][3] = {
        { 1, 0, 0}, {-1, 0, 0},
        { 0, 1, 0}, { 0,-1, 0},
        { 0, 0, 1}, { 0, 0,-1}
    };
    int idx = 0;
    for (int fi = 0; fi < 6; ++fi) {
        // For each target Y axis (4 choices: remaining axes excluding ±forward)
        for (int gi = 0; gi < 6; ++gi) {
            // Skip if same axis (±) as forward or opposite of forward
            if (gi == fi || gi == ((fi ^ 1))) continue;
            // Compute target Z = forward × up (cross product)
            int fx = axes[fi][0], fy = axes[fi][1], fz = axes[fi][2];
            int ux = axes[gi][0], uy = axes[gi][1], uz = axes[gi][2];
            int vx = fy * uz - fz * uy;
            int vy = fz * ux - fx * uz;
            int vz = fx * uy - fy * ux;
            // Build rotation matrix: columns are target X, Y, Z axes
            // R[i][j] = basis_i · (R * basis_j) = basis_i · target_axis[j]
            // The rotation maps old basis to new basis:
            // R * e0 = targetX, R * e1 = targetY, R * e2 = targetZ
            // So R[i][0] = targetX[i], R[i][1] = targetY[i], R[i][2] = targetZ[i]
            out[idx][0][0] = fx; out[idx][0][1] = ux; out[idx][0][2] = vx;
            out[idx][1][0] = fy; out[idx][1][1] = uy; out[idx][1][2] = vy;
            out[idx][2][0] = fz; out[idx][2][1] = uz; out[idx][2][2] = vz;
            ++idx;
        }
    }
}

void box_solver::precomputeRotations() {
    // Generate all 24 unique rotation matrices
    Mat3 all_rots[24];
    generate24Rotations(all_rots);

    for (int fig = 0; fig < 8; ++fig) {
        // Use temp storage for deduplication, then compact into precomputed_rotations
        FigureRotation tmp[24];
        int tmp_count = 0;
        for (int r = 0; r < 24; ++r) {
            const Mat3 &m = all_rots[r];
            auto &out = tmp[r];
            out.n_cubes = figures[fig].n_cubes;
            for (int c = 0; c < out.n_cubes; ++c) {
                out.state[c] = figures[fig].cubes[c].c.getState();
                // Transform position: p' = M * p
                out.pos[c][0] = m[0][0]*figures[fig].cubes[c].pos[0] + m[0][1]*figures[fig].cubes[c].pos[1] + m[0][2]*figures[fig].cubes[c].pos[2];
                out.pos[c][1] = m[1][0]*figures[fig].cubes[c].pos[0] + m[1][1]*figures[fig].cubes[c].pos[1] + m[1][2]*figures[fig].cubes[c].pos[2];
                out.pos[c][2] = m[2][0]*figures[fig].cubes[c].pos[0] + m[2][1]*figures[fig].cubes[c].pos[1] + m[2][2]*figures[fig].cubes[c].pos[2];
                // Transform direction if half-cube
                if (figures[fig].cubes[c].c.isHalf()) {
                    const int *dv = figures[fig].cubes[c].c.getVector();
                    out.dir[c][0] = m[0][0]*dv[0] + m[0][1]*dv[1] + m[0][2]*dv[2];
                    out.dir[c][1] = m[1][0]*dv[0] + m[1][1]*dv[1] + m[1][2]*dv[2];
                    out.dir[c][2] = m[2][0]*dv[0] + m[2][1]*dv[1] + m[2][2]*dv[2];
                } else {
                    out.dir[c][0] = out.dir[c][1] = out.dir[c][2] = 0;
                }
            }
            // Store rotated figure direction vectors
            // Base directions are (0,1,0) for both direction[0] and direction[1]
            // M * (0,1,0) = second column of M
            out.dir0[0] = m[0][1]; out.dir0[1] = m[1][1]; out.dir0[2] = m[2][1];
            out.dir1[0] = m[0][1]; out.dir1[1] = m[1][1]; out.dir1[2] = m[2][1];
            // Deduplicate: skip if this orientation matches an earlier one
            bool unique = true;
            for (int r2 = 0; r2 < tmp_count; ++r2) {
                if (memcmp(&tmp[r], &tmp[r2], sizeof(FigureRotation)) == 0) {
                    unique = false;
                    break;
                }
            }
            if (unique) {
                memcpy(&tmp[tmp_count++], &tmp[r], sizeof(FigureRotation));
            }
        }
        // Copy compacted unique rotations into precomputed_rotations
        valid_rotation_count[fig] = tmp_count;
        memcpy(precomputed_rotations[fig], tmp, sizeof(FigureRotation) * tmp_count);
    }
}

void box_solver::constructFigureFromRotation(unsigned int figIdx, int rotIdx, figure &out)
{
    out.reset();
    out.n_cubes = 0;

    const auto &rot = precomputed_rotations[figIdx][rotIdx];
    out.direction[0].resize(3);
    out.direction[1].resize(3);
    // Use precomputed rotated direction vectors (not hardcoded 0,1,0)
    out.direction[0][0] = rot.dir0[0]; out.direction[0][1] = rot.dir0[1]; out.direction[0][2] = rot.dir0[2];
    out.direction[1][0] = rot.dir1[0]; out.direction[1][1] = rot.dir1[1]; out.direction[1][2] = rot.dir1[2];
    for (int c = 0; c < rot.n_cubes; ++c) {
        figure_cube fc;
        fc.pos[0] = rot.pos[c][0];
        fc.pos[1] = rot.pos[c][1];
        fc.pos[2] = rot.pos[c][2];
        fc.c.setState((cell_t)rot.state[c]);
        if (rot.state[c] == cell_half) {
            int v[3] = { rot.dir[c][0], rot.dir[c][1], rot.dir[c][2] };
            fc.c.setVector(v);
        }
        out.addFigureCube(fc);
    }
}

void box_solver::stopComputing()
{
    stopFlag.store(true, std::memory_order_relaxed);
    poolCv.notify_all();
}

void box_solver::updateStatus()
{
    // Atomically request a status update: IDLE → REQUESTED (no-op if already REQUESTED or WRITING)
    int expected = 0;
    updateStatusFlag.compare_exchange_strong(expected, 1, std::memory_order_relaxed, std::memory_order_relaxed);
}

void box_solver::setRotateVector(std::array<std::array<int,3>,3> &m,
                                 const std::array<int,3> &a,
                                 const std::array<int,3> &b)
{
    // Column 0 = a, Column 1 = b, Column 2 = a x b (cross product)
    m[0][0] = a[0]; m[1][0] = a[1]; m[2][0] = a[2];
    m[0][1] = b[0]; m[1][1] = b[1]; m[2][1] = b[2];
    m[0][2] = a[1]*b[2] - a[2]*b[1];
    m[1][2] = a[2]*b[0] - a[0]*b[2];
    m[2][2] = a[0]*b[1] - a[1]*b[0];
}


void box_solver::initSolution() {
    initFigures();

    // Precomputed rotation matrices (column-major: col0=col[0], col1=col[1], col2=col[2])
    auto rotZ90 = []() -> Mat3 {
        Mat3 r;
        r[0] = {{ 0, -1,  0}}; r[1] = {{ 1,  0,  0}}; r[2] = {{ 0,  0,  1}};
        return r;
    }();
    auto rotY90 = []() -> Mat3 {
        Mat3 r;
        r[0] = {{ 0,  0,  1}}; r[1] = {{ 0,  1,  0}}; r[2] = {{-1,  0,  0}};
        return r;
    }();
    auto rotZm90 = []() -> Mat3 {
        Mat3 r;
        r[0] = {{ 0,  1,  0}}; r[1] = {{-1,  0,  0}}; r[2] = {{ 0,  0,  1}};
        return r;
    }();
    auto rotXm90 = []() -> Mat3 {
        Mat3 r;
        r[0] = {{ 1,  0,  0}}; r[1] = {{ 0,  0,  1}}; r[2] = {{ 0, -1,  0}};
        return r;
    }();

    solution[0] = figures[0];
    solution[0].rotate(rotZ90);
    solution_pos[0][0] = 0;
    solution_pos[0][1] = 0;
    solution_pos[0][2] = 0;

    solution[1] = figures[1];
    solution[1].rotate(rotY90);
    solution_pos[1][0] = 2;
    solution_pos[1][1] = 1;
    solution_pos[1][2] = 0;

    solution[2] = figures[2];
    solution[2].rotate(rotZm90);
    solution_pos[2][0] = 3;
    solution_pos[2][1] = 2;
    solution_pos[2][2] = 0;

    solution[3] = figures[3];
    solution[3].rotate(rotZ90);
    solution_pos[3][0] = 1;
    solution_pos[3][1] = 0;
    solution_pos[3][2] = 0;

    solution[4] = figures[4];
    solution[4].rotate(rotZm90);
    solution_pos[4][0] = 0;
    solution_pos[4][1] = 2;
    solution_pos[4][2] = 1;

    solution[5] = figures[5];
    solution[5].rotate(rotZ90);
    solution_pos[5][0] = 2;
    solution_pos[5][1] = 0;
    solution_pos[5][2] = 1;

    solution[6] = figures[6];
    solution[6].rotate(rotXm90);
    solution_pos[6][0] = 2;
    solution_pos[6][1] = 1;
    solution_pos[6][2] = 1;

    solution[7] = figures[7];
    solution[7].rotate(rotZ90);
    solution_pos[7][0] = 1;
    solution_pos[7][1] = 2;
    solution_pos[7][2] = 0;

    GLWidget::setGLSolution(figures.size(), solution, solution_pos);
}
