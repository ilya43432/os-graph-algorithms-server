#pragma once
#include "Algorithms.hpp"
#include <vector>
#include <algorithm>
#include <string>
#include <sstream>
#include <set>

class SCC_algo : public Algorithms
{

private:
    // helper dfs function
    void dfs(int v,
             const std::vector<std::vector<int>> &adj,
             std::vector<bool> &visited,
             std::vector<int> &output) const
    {
        visited[v] = true;

        for (int u : adj[v])
        {
            if (!visited[u])
            {
                dfs(u, adj, visited, output);
            }
        }

        output.push_back(v);
    }

public:
    std::string execute(const Graph &graph) const;

    void strongly_connected_components(
        const std::vector<std::vector<int>> &adj,
        std::vector<std::vector<int>> &components,
        std::vector<std::vector<int>> &adj_cond) const;

    std::string algo_name() const
    {
        return "SCC";
    }
};
