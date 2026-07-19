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
#include "box_solver.hpp"
#include <boost/numeric/ublas/io.hpp>
#include <boost/foreach.hpp>
#include <iostream>
#include <mutex>

using namespace boost::numeric::ublas;

std::vector < box_solution > box_solution::solution_list;
std::mutex box_solution::solutionMutex;

size_t box_solution::solution_list_size()
{
    std::lock_guard<std::mutex> lock(solutionMutex);
    return solution_list.size();
}

bool box_solution::empty()
{
    std::lock_guard<std::mutex> lock(solutionMutex);
    return solution_list.empty();
}

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

bool box_solution::isUnique(std::vector < figure > nf, const std::vector < vector_int > np)
{
    std::lock_guard<std::mutex> lock(solutionMutex);

    // Need at least 2 figures for symmetry to matter
    if (nf.size() < 2) {
        return true;
    }

    BOOST_FOREACH(const box_solution & bs2, solution_list) {
        std::vector < figure > s;
        std::vector < vector_int > sp;
        bs2.to_vector(s, sp);

        for (unsigned int i = 0; i < nf.size(); ++i) {
            // Indices of remaining figures (excluding i)
            int n_remaining = static_cast<int>(nf.size()) - 1;
            std::vector<unsigned int> remIdx;
            remIdx.reserve(n_remaining);
            for (unsigned int j = 0; j < nf.size(); ++j) {
                if (j != i) remIdx.push_back(j);
            }

            for (int sym = 0; sym < 4; ++sym) {
                const BoxSymmetry &bs = boxSymmetries[sym];

                // Match remaining positions from existing solution to new solution
                int matched = 0;
                bool posMatch = true;
                std::vector<bool> npUsed(nf.size(), false);
                std::vector<int> matchTo(n_remaining, -1); // existing j -> new k

                for (int ri = 0; ri < n_remaining; ++ri) {
                    unsigned int j = remIdx[ri];
                    bool found = false;

                    for (unsigned int k = 0; k < np.size(); ++k) {
                        if (k == i || npUsed[k]) continue;

                        int symPos[3];
                        symPos[0] = bs.sign[0] * sp[j][0] + bs.offset[0];
                        symPos[1] = bs.sign[1] * sp[j][1] + bs.offset[1];
                        symPos[2] = bs.sign[2] * sp[j][2] + bs.offset[2];

                        if (symPos[0] == np[k][0] &&
                            symPos[1] == np[k][1] &&
                            symPos[2] == np[k][2]) {
                            npUsed[k] = true;
                            matchTo[ri] = static_cast<int>(k);
                            found = true;
                            break;
                        }
                    }

                    if (!found) {
                        posMatch = false;
                        break;
                    }
                    matched++;
                }

                if (!posMatch || matched != n_remaining) {
                    continue;
                }

                // Positions match under this symmetry — verify halfcube directions
                bool dirMatch = true;

                for (int ri = 0; ri < n_remaining; ++ri) {
                    unsigned int j = remIdx[ri];
                    int k = matchTo[ri];
                    if (k < 0) continue;

                    // Find first halfcube in existing figure j
                    int halfcubeDir[3] = {0, 0, 0};
                    bool foundHC = false;
                    for (unsigned int c = 0; c < s[j].n_cubes && !foundHC; ++c) {
                        if (s[j].cubes[c].c.isHalf()) {
                            const int *dv = s[j].cubes[c].c.getVector();
                            halfcubeDir[0] = dv[0];
                            halfcubeDir[1] = dv[1];
                            halfcubeDir[2] = dv[2];
                            foundHC = true;
                            break;
                        }
                    }

                    if (!foundHC) {
                        // Figure j has no halfcube — skip direction check
                        continue;
                    }

                    // Find corresponding figure in new solution
                    bool foundHC2 = false;
                    for (unsigned int c = 0; c < nf[k].n_cubes && !foundHC2; ++c) {
                        if (nf[k].cubes[c].c.isHalf()) {
                            const int *dv = nf[k].cubes[c].c.getVector();
                            // Transform direction under symmetry and compare
                            int symDir[3];
                            symDir[0] = bs.dsign[0] * halfcubeDir[0];
                            symDir[1] = bs.dsign[1] * halfcubeDir[1];
                            symDir[2] = bs.dsign[2] * halfcubeDir[2];

                            if (dv[0] != symDir[0] ||
                                dv[1] != symDir[1] ||
                                dv[2] != symDir[2]) {
                                dirMatch = false;
                            }
                            foundHC2 = true;
                            break;
                        }
                    }

                    if (!foundHC2) {
                        // Existing figure has halfcube, new figure doesn't
                        dirMatch = false;
                    }
                }

                if (posMatch && dirMatch) {
                    return false; // solutions equivalent under this symmetry
                }
            }
        }
    }

    return true; // unique
}


void box_solution::addSolution(std::vector < figure > nf, std::vector < vector_int > np)
{
    std::lock_guard<std::mutex> lock(solutionMutex);
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
