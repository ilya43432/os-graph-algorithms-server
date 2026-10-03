#pragma once
#include "Algorithms.hpp"
#include <string>

class MST_algo : public Algorithms
{
public:
    std::string execute(const Graph &graph) const;

    std::string algo_name() const
    {
        return "MST";
    }
};
