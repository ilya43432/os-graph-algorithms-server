#pragma once
#include <queue>
#include <functional>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

class ActiveGraph
{
private:
    // queue for the jobs
    // each job is a void() function
    std::queue<std::function<void()>> jobs_;

    // condition pthread to signal when the jobs are available
    pthread_cond_t cv_;

    // pthread mutex to protect access within the job queue
    pthread_mutex_t mx_;

    // bool flags-
    // is_active represents if the worker should keep "working"
    // started_ indicates if the thread was started
    bool is_active;
    bool started_;

    // worker thread handle
    pthread_t worker;


    // entry point for the pthread_create
    static void *threadLoop(void *arg)
    {
        //convert the void* to an ActiveGraph pointer
        ActiveGraph *self = static_cast<ActiveGraph *>(arg);
        
        // call the work_loop func that will run the worker loop
        self->work_loop();

        // thread returns void*
        return nullptr;
    }

    void work_loop()
    {
        while (1)
        {
            std::function<void()> job;

            // lock mutex
            // this helps to check or to throw out jobs
            pthread_mutex_lock(&mx_);

            // wait while the queue is empty
            // and the worker is still active
            while (is_active && jobs_.empty())
            {
                pthread_cond_wait(&cv_, &mx_);
            }

            // if there are no jobs left and a stop was requested
            // we exit the loop
            if (!is_active && jobs_.empty())
            {
                pthread_mutex_unlock(&mx_);
                break;
            }

            // pop the job from the queue
            job = jobs_.front();
            jobs_.pop();

            // unlock before executing the job
            pthread_mutex_unlock(&mx_);

            // execute the job outside of lock
            if (job)
            {
            job();
            }
        }
    }

public:
    ActiveGraph();
    ~ActiveGraph();

    // start the worker thread
    void start();

    // stop the worker thread
    void stop();

    // add a new job to the queue
    // j is the function to execute
    void add_job(std::function<void()> j);
};