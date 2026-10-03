#include "Euler_circle_algo.hpp"
#include <getopt.h>

/// @brief Hierholzer's algorithm to find Eulerian circuit
/// @param graph input graph
/// @param start_vertex starting vertex
/// @return vector of vertices in Eulerian cycle
std::vector<int> euler(const Graph &graph, int start_vertex)
{
    // copy of graph
    Graph temp = graph;
    // stack for the path
    std::stack<int> stack;
    // the circuit will be inserted into a vector
    std::vector<int> circuit;

    // push start vx into the stack
    stack.push(start_vertex);

    while (!stack.empty())
    {
        int v = stack.top();

        // check if no edges are left from v
        // if so we add it to the circuit
        if (temp.degree(v) == 0)
        {
            circuit.push_back(v);
            stack.pop();
        }
        else
        {
            // else, we take a any neighbor of v
            // remove the edge and continue
            int u = temp.get_any_neighbor(v);

            temp.remove_edge(v, u);
            stack.push(u);
        }
    }

    return circuit;
}

/// @brief return a string presentation of an euler circle in a graph
/// @param graph
/// @param start_vertex
/// @return string with the euler circle
std::string find_euler_circle_string(const Graph &graph)
{
   if (!has_euler_circle(graph))
    {
        return "No Eulerian cycle\n";
    }

    int start_vertex = -1;
    for (int i = 0; i < graph.get_num_vertices(); i++)
    {
        if (graph.degree(i) > 0 || (graph.directed() && graph.indegree(i) > 0))
        {
            start_vertex = i;
            break;
        }
    }

    if (start_vertex == -1)
    {
        return "No Eulerian cycle\n"; // no edges
    }

    // get the answer from the euler func above
    std::vector<int> answer = euler(graph, start_vertex);

    if (answer.empty())
    {
        return "No Eulerian cycle\n";
    }

    if (answer.front() != answer.back())
    {
        return "No Eulerian cycle\n";
    }

    std::string ans;

    for (auto i = answer.rbegin(); i != answer.rend(); i++)
    {
        ans += std::to_string((*i)) + " ";
    }

    ans += "\n";

    return ans;
}

/// @brief check if all vertices in the graph have even degree
/// @param g
/// @return true if all vertices have even degree
bool check_even_degree(const Graph &g)
{
    if (g.get_num_vertices() == 0)
    {
        return false;
    }

    for (int v = 0; v < g.get_num_vertices(); v++)
    {
        if (g.degree(v) % 2 != 0)
        {
            return false;
        }
    }
    return true;
}

/// @brief check if the graph is connected (ignoring isolated vertices)
/// @param g
/// @return true if connected
bool check_connectivity(const Graph &g)
{
    int n = g.get_num_vertices();
    if (n == 0)
    {
        return false;
    }

    // find first vertex with degree > 0
    int starting_vx = -1;
    for (int v = 0; v < n; v++)
    {
        if (g.degree(v) > 0)
        {
            starting_vx = v;
            break;
        }
    }

    // special case: no edges at all trivially Eulerian
    if (starting_vx == -1)
    {
        return true;
    }

    // we start a dfs traversal
    std::vector<bool> visited(n, false);
    std::stack<int> st;

    visited[starting_vx] = true;
    st.push(starting_vx);

    while (!st.empty())
    {
        int v = st.top();
        st.pop();

        for (int u = 0; u < n; u++)
        {
            if (g.is_connected(v, u) && !visited[u])
            {
                visited[u] = true;
                st.push(u);
            }
        }
    }

    // check that all non isolated vertices are visited
    for (int v = 0; v < n; v++)
    {
        if (g.degree(v) > 0 && !visited[v])
        {
            return false;
        }
    }

    return true;
}

/// @brief bool function that checks if a directed graph has an eulerian cycle
/// the requirement is if the in degree equals the out degree for every vertex in
/// graph g and we also need strong connectivity among the non isolated vertices
bool has_euler_circle_directed(const Graph &g)
{
    // in-degree must equal out-degree for every vertex
    for (int i = 0; i < g.get_num_vertices(); ++i)
    {
        if (g.indegree(i) != g.degree(i))
        {
            return false;
        }
    }

    // strong connectivity on the subgraph induced by non-isolated vertices
    const int n = g.get_num_vertices();

    int start = -1;

    for (int v = 0; v < n; ++v)
    {
        if (g.degree(v) > 0 || g.indegree(v) > 0)
        {
            start = v;
            break;
        }
    }
    if (start == -1)
    {
        return true; // empty graph (no edges) is trivially Eulerian
    }

    // dfs traversal
    std::vector<bool> vis_f(n, false);
    std::stack<int> st;

    st.push(start);

    vis_f[start] = true;

    while (!st.empty())
    {
        int v = st.top();
        st.pop();
        for (int u = 0; u < n; ++u)
        {
            if (!vis_f[u] && g.is_connected(v, u))
            {
                vis_f[u] = true;
                st.push(u);
            }
        }
    }

    // after the first dfs well do another dfs
    // so we will traverse the "transposed" graph g
    std::vector<bool> vis_r(n, false);
    st.push(start);

    vis_r[start] = true;

    while (!st.empty())
    {
        int v = st.top();
        st.pop();
        for (int u = 0; u < n; ++u)
        {
            if (!vis_r[u] && g.is_connected(u, v)) // edge u->v means v has reverse-edge to u
            {
                vis_r[u] = true;
                st.push(u);
            }
        }
    }

    // check that all non isolated vertices are visited in both the traversals
    for (int v = 0; v < n; ++v)
    {
        if ((g.degree(v) > 0 || g.indegree(v) > 0) && (!vis_f[v] || !vis_r[v]))
        {
            return false;
        }
    }
    return true;
}

/// @brief check if a graph has Eulerian cycle
/// @param g
/// @return true if Eulerian cycle exists
bool has_euler_circle(const Graph &g)
{
    if (g.directed())
    {
        return has_euler_circle_directed(g);
    }
    else
    {
        return check_connectivity(g) && check_even_degree(g);
    }
}