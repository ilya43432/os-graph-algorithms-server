#pragma once
#include "Algorithms.hpp"
#include <vector>
#include <string>

class HamCircuit_algo : public Algorithms {
public:
    std::string execute(const Graph& g) const override;
    std::string algo_name() const override;

private:
    bool isSafe(const Graph& g, int v, const std::vector<int>& path, int pos)const;
    bool ham_util(const Graph& g, std::vector<int>& path, std::vector<bool>& visited, int pos, int start) const;
};
