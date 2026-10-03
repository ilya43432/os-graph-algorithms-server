#pragma once
#include "../Part_1/Graph.hpp"
#include <iostream>
#include <algorithm>
#include <stack>
#include <vector>
#include <cstdlib>
#include <unistd.h>

Graph generate_graph(unsigned int seed, int num_edges, int num_vxs,  bool directed = false);
std::vector<std::pair<int, int>> get_possible_sets_directed(const Graph &g);
std::vector<std::pair<int, int>> get_possible_sets_undirected(const Graph &g);