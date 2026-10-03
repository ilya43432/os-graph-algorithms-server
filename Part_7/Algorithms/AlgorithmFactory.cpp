#include "AlgorithmFactory.hpp"


/// @brief the main function. ahdering to the factory pattern we have learned in the OOP course last semester
/// @param input_algo a const string that contains the algo input for example EULER. 
/// this needs to be in the correct form of course or else it wont work.
/// @return we return a unique pointer for the desired algo. using a unique ptr benefits us in
/// not managing the memory ourself and overall avoiding potential memory leaks
std::unique_ptr<Algorithms> AlgorithmFactory::create_algo(const std::string &input_algo)
{
    // check if input isnt empty
    if (input_algo.empty())
    {
        throw std::runtime_error("Algorithm name must be provided.");
    }

    // euler
    if (input_algo == "EULER")
    {
        return std::make_unique<EulerCircuit_algo>();
    }
    // mstw - minimum spanning tree weight
    else if (input_algo == "MSTW")
    {
        return std::make_unique<MST_algo>();
    }
    // strongly connected components
    else if (input_algo == "SCC")
    {
        return std::make_unique<SCC_algo>();
    }
    // ham circuit
    else if (input_algo == "HAM")
    {
        return std::make_unique<HamCircuit_algo>();
    }
    // finding max clique
    else if (input_algo == "CLIQUE")
    {
        return std::make_unique<MaxClique_algo>();
    }
    // else, the input isnt correct
    else
    {
        throw std::runtime_error("Unknown algorithm: " + input_algo);
    }
}