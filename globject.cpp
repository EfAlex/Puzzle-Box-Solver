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


#include "globject.hpp"
#include <iostream>
#include <boost/foreach.hpp>

globject::globject()
{
    pos[0] = pos[1] = pos[2] = 0;
    objects.resize(0);
}

void globject::draw()
{
    //std::cout << "globject draw" << std::endl;
    BOOST_FOREACH( globject &o, objects) {
        o.draw();
    }
}

globject * globject::new_clone(globject &other)
{
    return other.clone();
}

globject * globject::clone()
{
    return do_clone();
}

globject * globject::do_clone()
{
    return new globject(*this);
}

