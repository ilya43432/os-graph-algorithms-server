#pragma once
#include "../Part_1/Graph.hpp"
#include <stack>
#include <vector>
#include <string>
#include <stdexcept>
#include <iostream>
#include <unistd.h>

std::vector<int> euler(const Graph& graph, int start_vertex);
std::string find_euler_circle_string(const Graph& g);
bool check_even_degree(const Graph& g);
bool check_connectivity(const Graph &g);
bool has_euler_circle(const Graph &g);
bool has_euler_circle_directed(const Graph &g);