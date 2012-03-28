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
#include <QThread>

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
    long count;
    boost::numeric::ublas::vector < long > depth;

    vector_int direction;
    std::vector < figure > figures, solution;
    std::vector < vector_int >solution_pos;
    std::vector < boost::numeric::ublas::matrix < int > >rotate_matrix;
    box b;
    bool stopComputingFlag;
    bool updateStatusFlag;

    void initFigures();
    int solve();
    int iterateFigure(unsigned int);
    void init();
    void initSolution();
    void setRotateVector(boost::numeric::ublas::matrix < int > & , int, int, int, int, int, int);
};

#endif // BOX_SOLVER_HPP
