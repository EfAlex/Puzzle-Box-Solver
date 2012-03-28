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


#ifndef BOX_SOLUTION_HPP
#define BOX_SOLUTION_HPP

#include "common.hpp"
#include "figure.hpp"
#include "box.hpp"
#include <iostream>

class box_solution {
  public:
    box_solution();
    box_solution(const box_solution &);
    box_solution(std::vector < figure >, const std::vector < vector_int >);

    static std::vector < box_solution > solution_list;

    static bool isUnique(const std::vector < figure > , const std::vector < vector_int > );
    static void addSolution(const std::vector < figure >, const std::vector < vector_int > );
    boost::ptr_vector < figure > solution;
    boost::ptr_vector < boost::ptr_vector < int > > solution_pos;
    void to_vector(std::vector < figure > & , std::vector < vector_int > & ) const;
    friend std::ostream & operator << (std::ostream &,
                const box_solution &);
    box_solution * new_clone(const box_solution &);
};

#endif                          // BOX_SOLUTION_HPP
