#include "HamCircuit.hpp"

std::string HamCircuit_algo::execute(const Graph &graph) const
{
    // how many vertices in the graph
    int size = graph.get_num_vertices();

    // keep the path we build (init with -1)
    std::vector<int> g_path(size, -1);
    // mark visited vertices we took already
    std::vector<bool> visited(size, false);

    // start at 0
    g_path[0] = 0;
    visited[0] = true;

    // try to build the cycle, if fail say no
    if (!ham_util(graph, g_path, visited, 1, 0))
    {
        return "no hamiltonian circuit found\n";
    }

    // build the answer string
    std::string ans;
    for (int i = 0; i < size; i++)
    {
        ans += std::to_string(g_path[i]) + " ";
    }
    ans += std::to_string(g_path[0]) + "\n";
    return ans;
}

std::string HamCircuit_algo::algo_name() const
{
    return "HAM";
}

bool HamCircuit_algo::isSafe(const Graph &g, int v, const std::vector<int> &path, int pos) const
{
    // must be an edge from previous
    if (!g.is_connected(path[pos - 1], v))
    {
        return false;
    }
    // must not already be in the path
    for (int i = 0; i < pos; i++)
    {
        if (path[i] == v)
        {
            return false;
        }
    }
    return true;
}

bool HamCircuit_algo::ham_util(const Graph &g,
                               std::vector<int> &path,
                               std::vector<bool> &visited,
                               int pos,
                               int start) const
{
    // how many vertices
    int n = g.get_num_vertices();

    if (pos == n)
    {
        // base case: last vertex must connect back to start
        return g.is_connected(path[pos - 1], start);
    }

    // try all vertices as next candidate
    for (int v = 0; v < n; v++)
    {
        if (!visited[v] && isSafe(g, v, path, pos))
        {
            // choose v
            path[pos] = v;
            visited[v] = true;

            // go deeper
            if (ham_util(g, path, visited, pos + 1, start))
            {
                return true;
            }

            // backtrack if fail
            visited[v] = false;
            path[pos] = -1;
        }
    }

    return false;
}
