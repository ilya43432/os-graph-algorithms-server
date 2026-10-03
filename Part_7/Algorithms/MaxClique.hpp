#pragma once
#include "Algorithms.hpp"
#include <vector>
#include <set>
#include <algorithm>

class MaxClique_algo : public Algorithms {
public:
    std::string algo_name() const { return "CLIQUE"; }

    std::string execute(const Graph& g) const;

private:
    void bronk(std::set<int>& R, std::set<int>& P, std::set<int>& X,
               const Graph& g, std::vector<std::set<int>>& cliques) const;
};