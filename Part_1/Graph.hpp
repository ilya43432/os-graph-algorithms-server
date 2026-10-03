#pragma once
#include <iostream>
#include "Node.hpp"
#include <vector>

class Graph
{
private:

    int num;

    std::vector<Node> nodes;

    bool is_directed_;

public:

    Graph() : num(0), is_directed_(false) {}
    

    Graph(int num, bool is_directed = false);

    void add_vertex(int id);

    void add_edge(int source, int target, int weight = 1);

    bool remove_edge(int source, int target);

    void remove_vertex(int id);

    bool directed() const { return is_directed_; }

    int get_num_vertices() const;

    size_t degree(int id) const;

    int get_any_neighbor(int id);

    bool contains_edge(int source, int target) const;

    bool is_connected(int source, int target) const;

    bool is_empty() const
    {
        return nodes.empty();
    }

    std::vector<std::pair<int, int>> get_weighted_neighbours(int id) const;


    size_t indegree(int vx_id) const;

    friend std::ostream &operator<<(std::ostream &os, const Graph &graph);
};
