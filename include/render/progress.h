/*
   Based on the Dartmouth Academic Ray Tracing Skeleton.

    Copyright (c) 2017-2021 by Wojciech Jarosz
*/
#pragma once

#include <atomic>
#include <thread>

class ProgressBar {
public:
    ProgressBar(int64_t nb_steps);
    ~ProgressBar() { set_done();  }

    void step(int64_t steps = 1)
    {
        m_steps_done += steps;
    }

    void set_done();
    int progress() const
    {
        return int(m_steps_done);
    }
    ProgressBar& operator++()
    {
        step();
        return *this;
    }
    ProgressBar& operator+=(int64_t steps)
    {
        step(steps);
        return *this;
    }
private:
    std::atomic<int64_t> m_steps_done;
    std::atomic<bool>    m_exit;
    std::thread          m_update_thread;
    int64_t              m_num_steps;
};