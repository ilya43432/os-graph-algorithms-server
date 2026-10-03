#pragma once
#include <iostream>
#include <string>
#include "../../Part_1/Graph.hpp"
#include "Algorithms.hpp"
#include "EulerCircuit.hpp"
#include "HamCircuit.hpp"
#include "MaxClique.hpp"
#include "MSTW.hpp"
#include "SCC.hpp"

#include <memory>

class AlgorithmFactory
{
public:
    // static main function for the factory pattern
    static std::unique_ptr<Algorithms> create_algo(const std::string& algo);
};
