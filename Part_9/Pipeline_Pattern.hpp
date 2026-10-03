#pragma once
#include <vector>
#include <functional>
#include "ActiveGraph.hpp"

class Pipeline
{
private:

    // vector of the stages
    // each stage is an ActiveGraph pointer
    std::vector<ActiveGraph *> stages;

public:
    // default constructor
    Pipeline() = default;

    // add a new stage to the pipeline
    void add_stage(ActiveGraph *s);

    // start all the stages
    // we spawn their worker threads
    void start();

    // stop all the stages
    // we join their worker threads
    void stop();

    //push the job to the first stage
    void push(const std::function<void()> &j);

    // push to a specific stage in the pipeline
    void push_to(size_t idx, const std::function<void()> &j);
};