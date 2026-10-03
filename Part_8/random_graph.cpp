#include "random_graph.hpp"
#include <getopt.h>
#include <iostream>
#include <cstdlib>
#include <stdexcept>

/// @brief main function
/// @param seed random seed
/// @param num_edges number of edges
/// @param num_vxs number of vertices
/// @param is_directed bool that states whether the graph is directed or undirected
/// @return returns a generated random graph
Graph generate_graph(unsigned int seed, int num_edges, int num_vxs, bool is_directed)
{
    // num vxs cant be negative or 0
    if (num_vxs <= 0)
    {
        throw std::invalid_argument("number of vertices must be non-negative");
    }

    // num edges cant be negative
    if (num_edges < 0)
    {
        throw std::invalid_argument("number of edges must be non-negative");
    }

    int max_edges;

    if (is_directed)
    {
        max_edges = num_vxs * (num_vxs - 1);
    }
    else
    {
        max_edges = (num_vxs * (num_vxs - 1)) / 2;
    }

    // check if the num of edges is greater than all the possible edges
    if (num_edges > max_edges)
    {
        throw std::invalid_argument("number of edges cant be higher than " + std::to_string(max_edges));
    }

    // initialiez empty graph
    Graph graph(num_vxs, is_directed);

    // build the possible pairs depending on the graphs direction
    std::vector<std::pair<int, int>> possible_pairs =
        (is_directed) ? get_possible_sets_directed(graph) : get_possible_sets_undirected(graph);

    std::vector<int> weights;

    weights.reserve(num_edges);

    srand(seed);

    for (int i = static_cast<int>(possible_pairs.size()) - 1; i > 0; --i)
    {
        int j = rand() % (i + 1);
        std::swap(possible_pairs[i], possible_pairs[j]);
    }

    for (int i = 0; i < num_edges; ++i)
    {
        weights.push_back(1 + rand() % 25);
    }


    for (int i = 0; i < num_edges; ++i)
    {
        const auto &edge = possible_pairs[i];
        graph.add_edge(edge.first, edge.second, weights[i]);
    }

    return graph;
}

/// @brief return all the possible edge pairs for a specified undirected graph
/// @param g the graph
/// @return  vector of candidate edge pairs
std::vector<std::pair<int, int>> get_possible_sets_undirected(const Graph &g)
{
    std::vector<std::pair<int, int>> ans;

    for (int i = 0; i < g.get_num_vertices(); i++)
    {
        for (int j = i + 1; j < g.get_num_vertices(); j++)
        {
            ans.emplace_back(i, j);
        }
    }

    return ans;
}

/// @brief return all the possible edge pairs for a specified directed graph
/// @param g the graph
/// @return  vector of candidate edge pairs
std::vector<std::pair<int, int>> get_possible_sets_directed(const Graph &g)
{
    std::vector<std::pair<int, int>> ans;

    for (int i = 0; i < g.get_num_vertices(); i++)
    {
        for (int j = 0; j < g.get_num_vertices(); j++)
        {
            if (i != j)
            {
                ans.emplace_back(i, j);
            }
        }
    }
    return ans;
}