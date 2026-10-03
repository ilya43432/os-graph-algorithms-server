#include "MaxClique.hpp"


/// @brief execute the Max Clique algorithm using Bron Kerbosch algorithm
/// @param g input graph
/// @return string representation of the maximum clique found
std::string MaxClique_algo::execute(const Graph &g) const
{
    int n = g.get_num_vertices();


    // vector that will hold all the cliques
    std::vector<std::set<int>> cliques;

    // R - empty set which is the current clique
    // P - all vertices to explore
    // X - empty set. vertices the are already processed
    std::set<int> R, P, X;

    for (int i = 0; i < n; i++)
    {
        P.insert(i);
    }

    // recurisve bron's algo
    bronk(R, P, X, g, cliques);

    // find maximum clique size and index
    int index_found = -1;

    size_t maxSize = 0;

    for (int i = 0; i < (int)cliques.size(); i++)
    {
        if (cliques[i].size() > maxSize)
        {
            maxSize = cliques[i].size();
            index_found = i;
        }
    }

    // we build the string representation of the clique vertices
    std::string clique_vxs = "{ ";

    if (index_found != -1)
    {
        int counter = 0;

        for (int v : cliques[index_found])
        {
            clique_vxs += std::to_string(v);
            if (++counter < (int)cliques[index_found].size())
                clique_vxs += ", ";
            else
                clique_vxs += " ";
        }
    }


    clique_vxs += "}";

    std::string ans = "Max Clique found: " + std::to_string(maxSize) + "\n"
    // Clique representation
    + "Clique vertices: " + clique_vxs + "\n";

    return ans;
}



/// @brief bron Kerbosch recursive backtracking algorithm
/// @param R current clique being built
/// @param P candidate vertices to add
/// @param X vertices already considered (to avoid duplicates)
/// @param g input graph
/// @param cliques storage for all maximal cliques found
void MaxClique_algo::bronk(std::set<int> &R, std::set<int> &P, std::set<int> &X,const Graph &g, std::vector<std::set<int>> &cliques) const
{
    // if p and x are empty
    // r is the maximal clique
    if (P.empty() && X.empty())
    {
        cliques.push_back(R);
        return;
    }

    // we copy p to avoid modifying during iteration
    auto Pcopy = P;

    for (int v : Pcopy)
    {
        // insert v to the current clique
        R.insert(v);

        // build the new candidates and exclusion sets
        // containing only neighbours of v
        std::set<int> Pnew, Xnew;

        for (int u : P) 
        {
            if (g.contains_edge(v, u))
            {
                Pnew.insert(u);
            }
        }

        for (int u : X) 
        {
            if (g.contains_edge(v, u)) 
            {
                Xnew.insert(u);
            }
        }

        // we recurse with the updated sets
        bronk(R, Pnew, Xnew, g, cliques);

        // backtrack
        R.erase(v);
        P.erase(v);
        X.insert(v);
    }
}