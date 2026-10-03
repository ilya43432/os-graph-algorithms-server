#include "Graph.hpp"
#include <algorithm>
#include <stdexcept>
#include <utility>
#include <cstdlib>
#include <ctime>

// constructor
Graph::Graph(int num, bool is_directed) : num(num), is_directed_(is_directed)
{
    // reserve num space for the vector
    nodes.reserve(num);
    for (int i = 0; i < num; ++i)
    {
        nodes.emplace_back(i);
    }
}

// get number of vertices
int Graph::get_num_vertices() const
{
    return num;
}

// add a vertex to the graph
void Graph::add_vertex(int id)
{
    for (const auto &vx : nodes)
    {
        if (vx.get_id() == id)
        {
            throw std::runtime_error("A vertex cannot be duplicated!");
        }
    }
    nodes.emplace_back(id);
    num++;
}

// add edge function - for building the graph
void Graph::add_edge(int source, int target, int weight)
{
    // temp node pointer for source and target
    Node *source_node = nullptr;
    Node *target_node = nullptr;

    for (auto &n : nodes)
    {
        if (n.get_id() == source)
        {
            // if the id equals the source then assign the source pointer to point at it
            source_node = &n;
        }
        // if the id equals to the target then assign the target pointer to point at it
        else if (n.get_id() == target)
        {
            target_node = &n;
        }
    }

    // pointers werent assigned -
    // source and node vxs are not found
    if (!source_node || !target_node)
    {
        throw std::runtime_error("Source or target vertex not found in the graph");
    }

    // edge already exists check
    if (source_node->contains_neighbor(target))
    {
        throw std::runtime_error("Edge already exists!");
    }

    // from the Node class - we add the target and the weight to his neighbours vector
    // which translates to adding an edge in the graph
    source_node->add_neighbor(target, weight);

    // if the graph is undirected we need to assign an edge from both source to target and vice versa
    if (!is_directed_)
    {
        target_node->add_neighbor(source, weight);
    }
}

// remove edge function
bool Graph::remove_edge(int source, int target)
{
    bool removed = false;

    // we have source and target nodes and the edge we want to remove between them
    for (auto &node : nodes)
    {
        // find the source in the graph using the nodes vector
        if (node.get_id() == source)
        {
            // get the neighbours of the source vx
            auto &neighbors = node.get_neighbors();
            for (auto it = neighbors.begin(); it != neighbors.end(); ++it)
            {
                // we use an iterator (vector iterator)
                // to find the target
                if (it->first == target)
                {
                    // erase the edge and then break
                    neighbors.erase(it);
                    removed = true;
                    break;
                }
            }
            break;
        }
    }

    if (!removed)
    {
        throw std::runtime_error("Edge is not found");
    }

    // if the graph is undirected we have to delete the edge from both sides
    // this is the exact same process as before
    if (!is_directed_)
    {
        for (auto &node : nodes)
        {
            if (node.get_id() == target)
            {
                auto &neighbors = node.get_neighbors();
                for (auto it = neighbors.begin(); it != neighbors.end(); ++it)
                {
                    if (it->first == source)
                    {
                        neighbors.erase(it);
                        break;
                    }
                }
                break;
            }
        }
    }
    return true;
}

// remove a vertex from the graph
void Graph::remove_vertex(int id)
{
    int index = -1;

    for (size_t i = 0; i < nodes.size(); i++)
    {
        if (nodes[i].get_id() == id)
        {
            index = static_cast<int>(i);
            break;
        }
    }

    if (index == -1)
    {
        throw std::runtime_error("Vertex not found");
    }

    // Remove all edges pointing to this vertex
    for (auto &node : nodes)
    {
        for (auto it = node.get_neighbors().begin(); it != node.get_neighbors().end();)
        {
            if (it->first == id)
                it = node.get_neighbors().erase(it);
            else
                ++it;
        }
    }

    // Remove the vertex itself
    nodes.erase(nodes.begin() + index);
    num--;
}

/// @brief calculate the degree of a vertex
/// @param id of vertex
/// @return degree sum of the specified vertex
size_t Graph::degree(int id) const
{
    for (const auto &n : nodes)
    {
        if (id == n.get_id())
        {
            return n.get_neighbors().size();
        }
    }
    throw std::runtime_error("Vertex not found");
}

/// @brief calculate the in degree of a specified vertex
/// @param vx_id the id of the vertex
/// @return amount of neighbours
size_t Graph::indegree(int vx_id) const
{
    size_t ans = 0;
    for (const auto &node : nodes)
    {
        for (const auto &neighbour : node.get_neighbors())
        {
            if (neighbour.first == vx_id)
            {
                ans++;
            }
        }
    }
    return ans;
}

// get any neighbour of a specific vertex
int Graph::get_any_neighbor(int id)
{
    for (const auto &node : nodes)
    {
        if (node.get_id() == id)
        {
            if (!node.get_neighbors().empty())
            {
                return node.get_neighbors().front().first;
            }
            else
            {
                throw std::runtime_error("No neighbors for this vertex");
            }
        }
    }
    throw std::runtime_error("Vertex not found");
}

// check if a graph contains an edge within 2 specified vxs: source & target
bool Graph::contains_edge(int source, int target) const
{
    for (const auto &n : nodes)
    {
        if (n.get_id() == source)
        {
            for (const auto &neighbour : n.get_neighbors())
            {
                if (neighbour.first == target)
                {
                    return true;
                }
            }
        }
    }
    return false;
}

// return a vector of neighbour pairs with weights of a specific vertex
std::vector<std::pair<int, int>> Graph::get_weighted_neighbours(int v) const
{
    for (const auto &node : nodes)
    {
        if (node.get_id() == v)
        {
            return node.get_neighbors();
        }
    }
    throw std::runtime_error("vertex not found");
}

// Is there an edge from source to target
bool Graph::is_connected(int source, int target) const
{
    for (const auto &node : nodes)
    {
        if (node.get_id() == source)
        {
            for (const auto &p : node.get_neighbors())
            {
                if (p.first == target)
                    return true;
            }
        }
    }
    return false;
}

// print the graph using ostream
// this is a friend function
std::ostream &operator<<(std::ostream &os, const Graph &graph)
{

    if (!graph.directed())
    {
        for (const auto &node : graph.nodes)
        {
            os << "Node: " << node.get_id() << " neighbors: ";
            for (auto &p : node.get_neighbors())
            {
                os << "(" << p.first << ", w=" << p.second << ") ";
            }
            os << "\n";
        }
        return os;
    }
    else
    {
        for (const auto &node : graph.nodes)
        {
            os << "Node: " << node.get_id() << " neighbors: ";
            for (auto &n : node.get_neighbors())
            {
                // directed edge
                os << "(->" << n.first << ", w=" << n.second << ") ";
            }
            os << "\n";
        }
        return os;
    }
}