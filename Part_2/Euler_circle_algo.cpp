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

int main(int argc, char *argv[])
{
    // args - ./Euler_circle_algo -v <num vertices> -e <num edges> -du <D|U>
    // enter number of EDGES
    // then input for the edges
    // calc euler graph (if exists)
    // return ans using the ostream function in the graph

    // we can use the getopt(3) function

    int num_vxs = 0;
    int num_edges = 0;

    // directed
    bool d_activate = false;

    // undirected
    bool u_activate = false;

    bool is_directed = false;

    // opt
    int opt;

    if (argc < 5) // need at least -v <num> -e <num> -U|D <Directed|Undirected>
    {
        std::cerr << "Usage is: " << argv[0]
                  << " -v <vertices> -e <edges> -(d|u) <directed | undirected>\n";
        return 1;
    }

    // getopt
    while ((opt = getopt(argc, argv, "v:e:du")) != -1)
    {
        switch (opt)
        {
        case 'v': // vertices
            num_vxs = std::atoi(optarg);
            break;
        case 'e': // edges
            num_edges = std::atoi(optarg);
            break;
        case 'd': // direction - Directed
            is_directed = true;
            d_activate = true;
            break;
        case 'u': // direction - Undirected
            is_directed = false;
            u_activate = true;
            break;
        default:
            std::cerr << "Usage: " << argv[0]
                      << " -v <vertices> -e <edges> ( -d | -u )\n";
            return 1;
        }
    }

    // cant have a graph both directed and undirected
    if (d_activate && u_activate)
    {
        std::cerr << "only -d or -u is required!\n";
        return 1;
    }

    if (!d_activate && !u_activate)
    {
        std::cerr << "you must specify either -d (directed) or -u (undirected)\n";
        return 1;
    }

    // graph cant have a negative number of vertices or 0
    if (num_vxs <= 0)
    {
        std::cerr << "Number of vertices must be positive!\n";
        return 1;
    }

    // graph cant have a negative number of edges
    if (num_edges < 0)
    {
        std::cerr << "Number of edges must be positive!\n";
        return 1;
    }

    // we calculate the total max edges the graph g can have using the number of vertices
    int max_edges;

    int v = num_vxs;

    // for directed graphs
    if (is_directed)
    {
        max_edges = v * (v - 1);
    }
    else // undirected graphs
    {
        max_edges = (v * (v - 1)) / 2;
    }

    // if greater.. this is indeed a logical error
    if (num_edges > max_edges)
    {
        std::cerr << "number of edges cant be higher than " << max_edges << "\n";
        return 1;
    }

    int profile_test = 1000000;

    // initialize graph
    Graph g(num_vxs, is_directed);

    // user will input the edges
    // format is <source vx num> <target vx num> <edge weight>
    std::cout << "Enter edges: " << std::endl;
    std::cout << "The format is - {source target weight}" << std::endl;

    for (int i = 0; i < num_edges; i++)
    {
        try
        {
            // uninitiallized
            int source = -1;
            int target = -1;

            // default
            int weight = 1;

            // check proper input
            if (!(std::cin >> source >> target >> weight))
            {
                throw std::runtime_error("bad input line");
            }

            // check bounds
            if (source < 0 || source >= num_vxs || target < 0 || target >= num_vxs)
            {
                throw std::runtime_error("out of bounds");
            }

            // self loops are not allowed in our graphs
            if (source == target)
            {
                throw std::invalid_argument("self loops are not allowed!");
            }

            // for simplicity, we have chose the weights to range [1,25]
            if (weight < 1 || weight > 25)
            {
                throw std::invalid_argument("weight must be in [1,25]");
            }

            if (g.is_connected(source, target))
            {
                throw std::runtime_error("duplicate edge ignored");
            }

            // functionallity accounts for both undirected and directed
            // this inorder to avoid duplication
            g.add_edge(source, target, weight);
        }
        catch (const std::exception &ex)
        { // catch any exception during calculation process
            std::cerr << ex.what() << "\n";
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            i--;
            continue;
        }
    }

    while (profile_test > 0)
    {
        std::cout << "Euler Circle: " << find_euler_circle_string(g);
        profile_test--;
    }

    return 0;
}