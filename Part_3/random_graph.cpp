#include "random_graph.hpp"
#include "Euler_circle_algo.hpp"
#include <getopt.h>
#include <iostream>
#include <cstdlib>
#include <stdexcept>
#include <vector>
#include <utility>
#include <algorithm>

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

    // initialiez empty graph
    Graph graph(num_vxs, is_directed);
    try
    {
        srand(seed);

        int max_edges;

        if (is_directed)
        {
            max_edges = num_vxs * (num_vxs - 1);
        }
        else
        {
            max_edges = (num_vxs * (num_vxs - 1)) / 2;
        }

        if (num_edges > max_edges)
        {
            throw std::invalid_argument("number of edges cant be higher than " + std::to_string(max_edges));
        }

        // build the possible pairs depending on the graphs direction
        std::vector<std::pair<int, int>> possible_pairs =
            (is_directed) ? get_possible_sets_directed(graph) : get_possible_sets_undirected(graph);

        // shuffle the pairs
        for (int i = static_cast<int>(possible_pairs.size()) - 1; i > 0; --i)
        {
            int j = rand() % (i + 1);
            std::swap(possible_pairs[i], possible_pairs[j]);
        }

        // pick the first edges num_edges times
        for (int i = 0; i < num_edges; ++i)
        {
            const auto &edge = possible_pairs[i];
            int random_weight = 1 + rand() % 25;
            graph.add_edge(edge.first, edge.second, random_weight);
        }
    }

    catch (const std::exception &e)
    { // exception occurs during generation process
        std::cout << "error generating a random graph: " << e.what() << std::endl;
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

            if (!g.contains_edge(i, j))
            {
                ans.emplace_back(i, j);
            }
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
            if (i == j)
            {
                continue;
            }

            if (!g.contains_edge(i, j))
            {
                ans.emplace_back(i, j);
            }
        }
    }
    return ans;
}

int main(int argc, char *argv[])
{

    try
    {
        int num_edges = 0;
        int num_vxs = 0;
        unsigned int seed = 0;
        int opt;
        bool is_directed = false;

        bool d_activate = false;
        bool u_activate = false;

        // need 8 arguments
        if (argc < 8) // need at least -v <num> -e <num> -s <seed>
        {
            std::cerr << "Missing parameters. Usage: " << argv[0]
                      << " -v <vertices> -e <edges> -s <seed> -(d|u) <directed | undirected>\n";
            return 1;
        }

        // we use getopt as required
        while ((opt = getopt(argc, argv, "v:e:s:du")) != -1)
        {
            switch (opt)
            {
            case 'v': // vertices
                num_vxs = std::atoi(optarg);
                break;
            case 'e': // edges
                num_edges = std::atoi(optarg);
                break;
            case 's': // seed
                seed = static_cast<unsigned int>(std::atoi(optarg));
                break;
            case 'd': // graphs direction - Directed
                is_directed = true;
                d_activate = true;
                break;
            case 'u': // graphs direction - Undirected
                is_directed = false;
                u_activate = true;
                break;
            default:
                std::cerr << "Usage: " << argv[0]
                          << " -v <vertices> -e <edges> -s <seed> ( -d | -u )\n";
                return 1;
            }
        }

        // cant have both directed undirected graph
        if (d_activate && u_activate)
        {
            std::cerr << "only -d or -u is required!";
            return 1;
        }

        // call the generate_graph function
        Graph g = generate_graph(seed, num_edges, num_vxs, is_directed);

        std::cout << "Your generated graph:\n"
                  << g << std::endl;

        std::cout << find_euler_circle_string(g);
    }
    // we enconunter an exception during generation
    catch (const std::exception &ex)
    {
        std::cout << ex.what() << std::endl;
    }

    return 0;
}
