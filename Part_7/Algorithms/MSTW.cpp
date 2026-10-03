#include "MSTW.hpp"
#include <queue>
#include <climits>
#include <sstream>

/// @brief utility function that finds the vertex with the smallest key value
/// from the set of vertices that are not yet included in the mst
int min_key(const std::vector<int> &key, const std::vector<bool> &mst_set)
{
    int min = INT_MAX;
    int min_index = -1;

    for (int i = 0; i < static_cast<int>(key.size()); i++)
    {
        if (!mst_set[i] && key[i] < min)
        {
            min = key[i];
            min_index = i;
        }
    }

    return min_index;
}

/// @brief this is prim's algorithm
/// @param graph input graph
/// @return string representation of mstw
std::string MST_algo::execute(const Graph &graph) const
{
    int n = graph.get_num_vertices();

    if (n == 0)
    {
        return "Graph is empty\n";
    }

    // quick connectivity check (undirected graphs only)

    // we do a simple dfs to verify connectivity
    std::vector<bool> visited(n, false);
    std::vector<int> stack;
    stack.push_back(0);

    visited[0] = true;

    while (!stack.empty())
    {
        int v = stack.back();
        stack.pop_back();
        for (const auto &p : graph.get_weighted_neighbours(v))
        {
            int u = p.first;
            if (!visited[u])
            {
                visited[u] = true;
                stack.push_back(u);
            }
        }
    }
    for (int v = 0; v < n; ++v)
    {
        if (!visited[v])
        {
            return "Graph is disconnected and therefore no MST exists.\n";
        }
    }

    // parent vector
    // parent[i] will store the parent of vertex i in the mst
    std::vector<int> parent(n);

    // vector to hold the minimum edge weight to connect each vertex i
    std::vector<int> key(n, INT_MAX);

    // bool vector that states whether vertex i is already in the mst
    std::vector<bool> mst_set(n, false);

    // start from 0
    key[0] = 0;
    parent[0] = -1;

    // build mst
    for (int i = 0; i < n - 1; i++)
    {
        int u = min_key(key, mst_set);
        // check for error
        if (u == -1)
        {
            break;
        }

        // in the mst now
        mst_set[u] = true;

        // update the key and the parent for neighbours of u
        for (const auto &neighbor : graph.get_weighted_neighbours(u))
        {
            int v = neighbor.first;
            int weight = neighbor.second;

            if (!mst_set[v] && weight < key[v])
            {
                key[v] = weight;
                parent[v] = u;
            }
        }
    }

    Graph mst(n, false);

    // construct the mst from the parents vector
    for (int v = 1; v < n; v++)
    {
        int u = parent[v];
        // search for the weight in original graph
        int w = -1;
        for (const auto &p : graph.get_weighted_neighbours(u))
        {
            if (p.first == v)
            {
                w = p.second;
                break;
            }
        }
        if (w != -1)
            mst.add_edge(u, v, w);
    }

    // return mst as a string
    std::ostringstream os;
    os << mst;
    return os.str();
}
