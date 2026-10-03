#pragma once
#include <vector>

class Node
{
private:
    // id of the node
    int id;
    // vector that contains pairs of negihbours for each vertex - <id of the neighbour, edge weight>
    std::vector<std::pair<int, int>> neighbors;

public:
    // contructor
    Node(int id) : id(id) {}

    // check if the node is connected to a specified target node
    bool contains_neighbor(int nid) const
    {
        for (size_t i = 0; i < neighbors.size(); ++i)
        {
            if (neighbors[i].first == nid)
            {
                return true;
            }
        }
        return false;
    }

    // const version - return the nodes neighbours
    const std::vector<std::pair<int,int>>& get_neighbors() const
    {
        return neighbors;
    }

    // get the nodes neighbours 
    std::vector<std::pair<int,int>>& get_neighbors()
    {
        return neighbors;
    }

    // add a neighbour to the vector of pairs
    void add_neighbor(int nid, int weight)
    {
        // add the neighbour and the edge weight
        neighbors.emplace_back(nid,weight);
    }

    // id of the node
    int get_id() const
    {
        return id;
    }
};