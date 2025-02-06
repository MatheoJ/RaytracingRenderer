/*
   Based on the Dartmouth Academic Ray Tracing Skeleton.

    Copyright (c) 2017-2021 by Wojciech Jarosz
*/
#include <render/progress.h>
#include <render/common.h>

#include <spdlog/stopwatch.h>
#include <spdlog/fmt/fmt.h>

#include <iostream>
#include <chrono>
#include <algorithm>
#include <string>

std::string time_string(double time, int precision = 2)
{
    if (std::isnan(time) || std::isinf(time))
        return "inf";

    int seconds = int(time / 1000);
    int minutes = seconds / 60;
    int hours = minutes / 60;
    int days = hours / 24;

    if (days > 0)
        return fmt::format("{}d:{:0>2}h:{:0>2}m:{:0>2}s", days, hours % 24, minutes % 60, seconds % 60);
    else if (hours > 0)
        return fmt::format("{}h:{:0>2}m:{:0>2}s", hours % 24, minutes % 60, seconds % 60);
    else if (minutes > 0)
        return fmt::format("{}m:{:0>2}s", minutes % 60, seconds % 60);
    else if (seconds > 0)
        return fmt::format("{:.3f}s", time / 1000);
    else
        return fmt::format("{}ms", int(time));
}

ProgressBar::ProgressBar(int64_t nb_steps) {
	m_exit = false;
	m_num_steps = nb_steps;

	m_update_thread = std::thread(
		[this]()
	{
		spdlog::stopwatch timer;
		bool done = false;
		do {
			// Calcule temps et progression
            double elp = timer.elapsed().count() * 1000.;
			double fraction = clamp(double(m_steps_done) / double(m_num_steps), 0.0, 1.0);

            // Fini?
            done = m_exit || fraction >= 1.f;


            // Estimation du temps restant
			double eta = elp / fraction - elp;
			auto time_text = fmt::format(" ({}/{})", time_string(elp), time_string(std::max(0., elp + eta)));
            
            std::cout << "\r" << fmt::format("{:>4d}", int(fraction * 100)) << "% completed: [";
            std::cout << std::string(std::size_t(fraction / 0.05), '#') << std::string(std::size_t(20 - fraction / 0.05), ' ') << "]";
            std::cout << time_text << "    ";

            if (done) {
                std::cout << "\n";
            }

            if (!done)
            {
                using namespace std::chrono_literals;

                // gradually lengthening the update interval
                std::this_thread::sleep_for(std::clamp(std::chrono::milliseconds(int(elp * 0.01)), 40ms, 10000ms));
            }
		} while (!done);
	});
}

void ProgressBar::set_done() {
    m_steps_done = m_num_steps;
    m_exit = true;

    if (m_update_thread.joinable())
        m_update_thread.join();
}