#include "SCC.hpp"

std::string SCC_algo::execute(const Graph &graph) const
{
    int n = graph.get_num_vertices();
    std::vector<std::vector<int>> adj(n);

    // build the adjancey list from the graph
    for (int u = 0; u < n; u++)
    {
        for (int v = 0; v < n; v++)
        {
            if (graph.is_connected(u, v))
            {
                adj[u].push_back(v);
            }
        }
    }

    // vector that represents vectors of scc components
    std::vector<std::vector<int>> components;

    // vector that represents vectors dag of the sccs
    std::vector<std::vector<int>> adj_cond;

    // run the scc decomposition
    strongly_connected_components(adj, components, adj_cond);

    std::ostringstream oss;

    // build the sccs using ostringstream
    oss << "SCCs found: " << components.size() << "\n";
    for (size_t i = 0; i < components.size(); i++)
    {
        oss << "Component " << i + 1 << ": ";
        for (int v : components[i])
            // well print all the vertices in this component
            oss << v << " ";
        oss << "\n";
    }
    return oss.str();
}


/// @brief Kosaraju’s algorithm implementation
/// Step 1: we run dfs on original graph to compute finishing order
/// Step 2: reverse the graph
/// Step 3: we run dfs on reversed graph in finishing order to extract SCCs
/// Step 4: build condensation graph (edges between components)
void SCC_algo::strongly_connected_components(
    const std::vector<std::vector<int>> &adj,
    std::vector<std::vector<int>> &components,
    std::vector<std::vector<int>> &adj_cond) const
{
    int n = adj.size();
    components.clear();
    adj_cond.clear();

    // vector to store the order of the vertices
    std::vector<int> order;

    // bool vector for visited vertices
    std::vector<bool> visited(n, false);

    // first DFS on the original graph
    // fills "order" with vertices by finish time
    for (int i = 0; i < n; i++)
    {
        if (!visited[i])
        {
            dfs(i, adj, visited, order);
        }
    }

    // build the reversed graph
    std::vector<std::vector<int>> adj_rev(n);
    for (int v = 0; v < n; v++)
    {
        for (int u : adj[v])
        {
            adj_rev[u].push_back(v);
        }
    }

    // reset visited for a second dfs
    visited.assign(n, false);

    // we can use the vector's reverse iterator
    // to reverse the order of vertices
    std::reverse(order.begin(), order.end());

    // component id for each vertex
    std::vector<int> comp_id(n, -1);

    // second dfs on the reversed graph
    for (int v : order)
    {
        if (!visited[v])
        {
            // vector that represents the component
            std::vector<int> component;
            
            // run 2nd dfs
            dfs(v, adj_rev, visited, component);

            // add component to the vector
            components.push_back(component);

            // assign SCC index to each vertex
            int cid = components.size() - 1;
            for (int u : component)
            {
                comp_id[u] = cid;
            }
        }
    }

    // construct the DAG graph
    // we need to avoid duplicate edges
    std::set<std::pair<int, int>> seen;

    adj_cond.assign(components.size(), {});

    for (int v = 0; v < n; v++)
    {
        for (int u : adj[v])
        {
            // add edge only if endpoints are in seperate sccs
            if (comp_id[v] != comp_id[u] && seen.insert({comp_id[v], comp_id[u]}).second)
            {
                adj_cond[comp_id[v]].push_back(comp_id[u]);
            }
        }
    }
}