#include "Pipeline_Pattern.hpp"

/// @brief add a new stage to the pipeline
/// @param s
void Pipeline::add_stage(ActiveGraph *s)
{
    stages.push_back(s);
}

/// @brief start all stages
void Pipeline::start()
{
    for (size_t i = 0; i < stages.size(); i++)
    {
        stages[i]->start();
    }
}

/// @brief stop all stages
void Pipeline::stop()
{
    for (size_t i = 0; i < stages.size(); i++)
    {
        stages[i]->stop();
    }
}

/// @brief push a job into the first stage
/// @param job
void Pipeline::push(const std::function<void()> &job)
{
    if (!stages.empty())
    {
        stages[0]->add_job(job);
    }
}

/// @brief push a job into a specific stage
/// @param idx stage index stats from 0
/// @param job callable to run
void Pipeline::push_to(size_t idx, const std::function<void()> &job)
{
    if (idx < stages.size())
    {
        stages[idx]->add_job(job);
    }
}