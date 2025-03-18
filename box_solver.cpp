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
#include <boost/numeric/ublas/io.hpp>
#include <limits.h>
#include <boost/foreach.hpp>
#include "box.hpp"
#include "figure.hpp"
#include "box_solution.hpp"
#include "glwidget.hpp"

using namespace boost::numeric::ublas;

void box_solver::init()
{
    count = 0;
    rotate_matrix.resize(0);
    matrix < int >r(3, 3);
    r(0, 0) =  1; r(0, 1) =  0; r(0, 2) =  0;
    r(1, 0) =  0; r(1, 1) =  0; r(1, 2) = -1;
    r(2, 0) =  0; r(2, 1) =  1; r(2, 2) =  0;
    rotate_matrix.push_back(r);
    r(0, 0) =  0; r(0, 1) =  0; r(0, 2) =  1;
    r(1, 0) =  0; r(1, 1) =  1; r(1, 2) =  0;
    r(2, 0) = -1; r(2, 1) =  0; r(2, 2) =  0;
    rotate_matrix.push_back(r);
    r(0, 0) =  0; r(0, 1) = -1; r(0, 2) =  0;
    r(1, 0) =  1; r(1, 1) =  0; r(1, 2) =  0;
    r(2, 0) =  0; r(2, 1) =  0; r(2, 2) =  1;
    rotate_matrix.push_back(r);
    depth.resize(8);
    for (unsigned int d = 0; d < depth.size(); ++d) {
        depth[d] = 0;
    }
}
box_solver::box_solver():check_count(10000/*00*/),stopComputingFlag(false),updateStatusFlag(false)
{
    init();
}

void box_solver::run()
{
    initFigures();
    solve();
}

int box_solver::iterateFigure(unsigned int i)
{
    if (stopComputingFlag) {
        return 0;
    }
    if (i >= figures.size()) {
        if (box_solution::isUnique(solution, solution_pos)) {
            // found solution!
            box_solution::addSolution(solution, solution_pos);
            GLWidget::gl_count = figures.size();
            GLWidget::gl_solution = solution;
            GLWidget::gl_solution_pos = solution_pos;
            GLWidget::gl_pos = box_solution::solution_list.size() - 1;
            emit drawBox();
            //stopComputingFlag = true;
        }
        return 0;
    }
    figure *f = &(figures[i]);
    for (int rx = 0; rx < 4; ++rx) {
        for (int ry = 0; ry < 4; ++ry) {
            for (int rz = 0; rz < 4; ++rz) {
                // find figure boundaries
                vector_int min_v(3);
                vector_int max_v(3);
                min_v[0] = min_v[1] = min_v[2] = INT_MAX;
                max_v[0] = max_v[1] = max_v[2] = INT_MIN;
                BOOST_FOREACH(figure_cube & c, f->cubes) {
                    min_v[0] = std::min(min_v[0], c.pos[0]);
                    max_v[0] = std::max(max_v[0], c.pos[0]);
                    min_v[1] = std::min(min_v[1], c.pos[1]);
                    max_v[1] = std::max(max_v[1], c.pos[1]);
                    min_v[2] = std::min(min_v[2], c.pos[2]);
                    max_v[2] = std::max(max_v[2], c.pos[2]);
                }
                if (max_v[0] - min_v[0] > 4 || max_v[1] - min_v[1] > 3 || max_v[2] - min_v[2] > 2) {
                    continue;
                }
                // Move figure in the box
                vector_int v(3);
                for (int x = -min_v[0]; x < /*-min_v[0] + 1*/4 - max_v[0]; ++x) {
                    v[0] = x;
                    for (int y = -min_v[1]; y < /*-min_v[1] + 1 */3 - max_v[1]; ++y) {
                        v[1] = y;
                        for (int z = -min_v[2]; z < /*-min_v[2] + 1 */2 - max_v[2]; ++z) {
                            v[2] = z;
                            if (b.addFigure(*f, v)) {
                                ++count;
                                ++depth[i];
                                solution[i] = *f;
                                solution_pos[i] = v;
                                if (count % check_count == 0 || updateStatusFlag) {
                                    updateStatusFlag = false;
                                    GLWidget::gl_count = i + 1;
                                    GLWidget::gl_solution = solution;
                                    GLWidget::gl_solution_pos = solution_pos;
                                    emit drawBox();
                                }
                                // do recursive call
                                iterateFigure(i + 1);
                                if (stopComputingFlag) {
                                    return 0;
                                }
                                if (!b.delFigure(*f, v)) {
                                    std::cerr << "Delete error!" << std:: endl;
                                    exit(1);
                                }
                            }
                        }
                    }
                }
                f->rotate(rotate_matrix[2]);
            }
            f->rotate(rotate_matrix[1]);
        }
        f->rotate(rotate_matrix[0]);
    }
    return 0;
}

int box_solver::solve()
{
    int res = iterateFigure(0);
    emit drawBox();
    return res;
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

void box_solver::stopComputing()
{
    stopComputingFlag = true;
}

void box_solver::updateStatus()
{
    updateStatusFlag = true;
}

void box_solver::setRotateVector(boost::numeric::ublas::matrix < int > & m, int x1, int y1, int z1, int x2, int y2 , int z2)
{
    m(0,0) = x1;
    m(1,0) = y1;
    m(2,0) = z1;

    m(0,1) = x2;
    m(1,1) = y2;
    m(2,1) = z2;

    m(0,2) = y1 * z2 - z1 * y2;
    m(1,2) = z1 * x2 - x1 * z2;
    m(2,2) = x1 * y2 - y1 * x2;
}


void box_solver::initSolution() {
    initFigures();
    matrix < int >r(3, 3);

    setRotateVector(r, 0,1,0,-1,0,0);
    solution[0] = figures[0];
    solution[0].rotate(r);
    solution_pos[0][0] = 0;
    solution_pos[0][1] = 0;
    solution_pos[0][2] = 0;

    setRotateVector(r, 1,0,0, 0,1,0);
    solution[1] = figures[1];
    solution[1].rotate(r);
    solution_pos[1][0] = 2;
    solution_pos[1][1] = 1;
    solution_pos[1][2] = 0;

    setRotateVector(r, -1,0,0, 0,0,-1);
    solution[2] = figures[2];
    solution[2].rotate(r);
    solution_pos[2][0] = 3;
    solution_pos[2][1] = 2;
    solution_pos[2][2] = 0;

    setRotateVector(r, 0,0,1, 1,0,0);
    solution[3] = figures[3];
    solution[3].rotate(r);
    solution_pos[3][0] = 1;
    solution_pos[3][1] = 0;
    solution_pos[3][2] = 0;


    setRotateVector(r, 0,0,-1, 0,1,0);
    solution[4] = figures[4];
    solution[4].rotate(r);
    solution_pos[4][0] = 0;
    solution_pos[4][1] = 2;
    solution_pos[4][2] = 1;

    setRotateVector(r, 0,1,0, 0,0,1);
    solution[5] = figures[5];
    solution[5].rotate(r);
    solution_pos[5][0] = 2;
    solution_pos[5][1] = 0;
    solution_pos[5][2] = 1;

    setRotateVector(r, 1,0,0, 0,-1,0);
    solution[6] = figures[6];
    solution[6].rotate(r);
    solution_pos[6][0] = 2;
    solution_pos[6][1] = 1;
    solution_pos[6][2] = 1;

    setRotateVector(r, 0,0,1, -1,0,0);
    solution[7] = figures[7];
    solution[7].rotate(r);
    solution_pos[7][0] = 1;
    solution_pos[7][1] = 2;
    solution_pos[7][2] = 0;

    GLWidget::gl_count = figures.size();
    GLWidget::gl_solution = solution;
    GLWidget::gl_solution_pos = solution_pos;
}