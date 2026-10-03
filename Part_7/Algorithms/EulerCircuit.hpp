#pragma once
#include "Algorithms.hpp"
#include <stack>
#include <vector>
#include <string>

class EulerCircuit_algo : public Algorithms
{
public:
    std::string execute(const Graph &g) const override;
    std::string algo_name() const override
    {
        return "EULER";
    }

    bool has_euler_circle_directed(const Graph &g) const;
    bool has_euler_circle(const Graph &g) const;
    std::vector<int> euler(const Graph &graph, int start_vertex) const;

    private : 
    bool check_even_degree(const Graph &g) const;
    bool check_connectivity(const Graph &g) const;
};
