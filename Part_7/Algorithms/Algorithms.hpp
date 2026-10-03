#pragma once
#include <iostream>
#include "../../Part_1/Graph.hpp"

class Algorithms
{
public:
    //strategy pattern -
    // note that this algorithms class is an abstract class.
    // for the pattern to work by definition we need a 
    // execute func for each algo inhereting from this class
    // each algo will have to override the execute function
    // and the algo name function 
    // we have made the destructor virtual (needed)
    virtual std::string execute(const Graph& g) const = 0;
    virtual std::string algo_name() const = 0;
    virtual ~Algorithms()=default;
};
