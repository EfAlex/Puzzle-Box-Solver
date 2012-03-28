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


#include "box_solution.hpp"
#include <boost/numeric/ublas/io.hpp>
#include <boost/foreach.hpp>
#include <iostream>

using namespace boost::numeric::ublas;

std::vector < box_solution > box_solution::solution_list;

box_solution::box_solution()
{
}

box_solution::box_solution(const box_solution &other)
{
    solution = other.solution;
    solution_pos = other.solution_pos;
}

box_solution::box_solution(std::vector < figure > f, const std::vector < vector_int >p)
{
    solution.resize(f.size());
    solution_pos.resize(p.size());
    for (size_t i = 0; i < f.size(); ++i) {
        solution.replace(i, new figure(f[i]));
        solution_pos.replace(i, new boost::ptr_vector <int> );
        solution_pos[i].resize(p[i].size());
        for (size_t j = 0; j < p[i].size(); ++j) {
            int *v = new int;
            *v = p[i][j];
            solution_pos[i].replace(j, v);
        }
    }
}

void box_solution::to_vector(std::vector < figure > & s, std::vector < vector_int > & sp) const {
    s.resize(solution.size());
    sp.resize(solution_pos.size());
    for (size_t i = 0; i < solution.size(); ++i) {
        s[i] = solution[i];
        sp[i].resize(solution_pos[i].size());
        for (size_t j = 0; j < solution_pos[i].size(); ++j) {
            sp[i][j] = solution_pos[i][j];
        }
    }
}

bool box_solution::isUnique(std::vector < figure > nf, const std::vector < vector_int >np)
{
    box b1;
    box_solution bs1(nf, np);
    for (unsigned int i = 0; i < nf.size(); ++i) {
        b1.addFigure(nf[i], np[i]);
    }
    BOOST_FOREACH(box_solution & bs2, solution_list) {
        box b2;
        // fill the box with all figures
        std::vector < figure > s;
        std::vector < vector_int > sp;
        bs2.to_vector(s, sp);
        for (unsigned int i = 0; i < bs2.solution.size(); ++i) {
            b2.addFigure(s[i], sp[i]);
        }
        // remove one figure from box and compare the boxes
        for (unsigned int i = 0; i < nf.size(); i++) {
            box cb1(b1);
            box cb2(b2);
            cb1.delFigure(nf[i], np[i]);
            cb2.delFigure(s[i], sp[i]);
            if (cb1 == cb2) {
                return false;
            }
            cb2.rotateZ();
            if (cb1 == cb2) {
                return false;
            }
            cb2.rotateX();
            if (cb1 == cb2) {
                return false;
            }
            cb2.rotateZ();
            if (cb1 == cb2) {
                return false;
            }
        }
    }
    return true;
}


void box_solution::addSolution(std::vector < figure > nf, std::vector < vector_int >np)
{
    box_solution bs(nf, np);
    solution_list.push_back(bs);
}

box_solution *box_solution::new_clone(const box_solution &other)
{
    return new box_solution(other);
}

std::ostream &operator <<(std::ostream &os,
        const box_solution & bs)
{
    std::vector < figure > s;
    std::vector < vector_int > sp;
    bs.to_vector(s, sp);
    for (unsigned int i = 0; i < bs.solution.size(); ++i) {
        os << "Item: " << i << " Position: " <<  sp[i] << " Direction[0]: " << s[i].direction[0] << " Direction[1]: " << s[i].direction[1] << std::endl;
    }
    return os;
}
