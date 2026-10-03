#pragma once
#include "../Part_1/Graph.hpp"
#include "Euler_circle_algo.hpp"
#include <iostream>
#include <algorithm>
#include <stack>
#include <vector>
#include <cstdlib>
#include <unistd.h>
#include <getopt.h>

// #random_graph.hpp for part 3

// a function to generate a random graph
Graph generate_graph(unsigned int seed, int num_edges, int num_vxs, bool is_directed);
std::vector<std::pair<int, int>> get_possible_sets_directed(const Graph &g);
std::vector<std::pair<int, int>> get_possible_sets_undirected(const Graph &g);