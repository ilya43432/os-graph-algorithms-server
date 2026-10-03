#include "ActiveGraph.hpp"

/// @brief  constructor
ActiveGraph::ActiveGraph() : is_active(false), started_(false)
{

    // init mutex for the job queue to synchronize
    pthread_mutex_init(&mx_, nullptr);
    // as well as a condition variable
    pthread_cond_init(&cv_, nullptr);
}

/// @brief
ActiveGraph::~ActiveGraph()
{
    // stop the workers loop
    stop();

    // destroy the pthread primitives
    pthread_mutex_destroy(&mx_);
    pthread_cond_destroy(&cv_);
}

/// @brief start the worker thread
void ActiveGraph::start()
{

    // lock mutex
    pthread_mutex_lock(&mx_);

    // check if already active
    if (is_active)
    {
        pthread_mutex_unlock(&mx_);
        return;
    }

    // if not active, set is_active flag to true
    is_active = true;

    // unlock mutex
    pthread_mutex_unlock(&mx_);

    // create the worker thread using the threadloop as an entry
    if (pthread_create(&worker, nullptr, &ActiveGraph::threadLoop, this) != 0)
    {
        perror("pthread_create failed");
        exit(1);
    }

    // thread has started
    started_ = true;
}

/// @brief stop the worker thread
void ActiveGraph::stop()
{
    // lock mutex
    pthread_mutex_lock(&mx_);

    // check if already stopped
    if (!is_active)
    {
        pthread_mutex_unlock(&mx_);
        return;
    }

    // is_active flag sets to false
    is_active = false;

    // wake up the worker if hes waiting on empty queue
    pthread_cond_broadcast(&cv_);
    pthread_mutex_unlock(&mx_);

    // if a thread was started
    // we join it to clean up
    if (started_)
    {
        pthread_join(worker, nullptr);
        started_ = false;
    }
}

/// @brief add a new job to the queue
/// @param j is the function
void ActiveGraph::add_job(std::function<void()> j)
{
    // lock mutex
    pthread_mutex_lock(&mx_);

    // enqueue the job
    jobs_.push(j);

    // signal the worker
    pthread_cond_signal(&cv_);

    // unlock mutex
    pthread_mutex_unlock(&mx_);
}