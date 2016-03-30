// mousse: CFD toolbox
// Copyright (C) 2011 OpenFOAM Foundation
// Copyright (C) 2016 mousse project

#include <new>
#include <iostream>
#include <cstdlib>


namespace mousse {

void newError()
{
  std::cerr <<
    "new cannot satisfy memory request.\n"
    "This does not necessarily mean you have run out of virtual memory.\n"
    "It could be due to a stack violation caused "
    "by e.g. bad use of pointers or an out of date shared library"
    << std::endl;
  ::abort();
}

void (*old_new_handler)() = std::set_new_handler(newError);

}  // namespace mousse
