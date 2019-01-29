#include <stdarg.h>
#include "utils.h"
#include "log/log.h"
#include <iostream>
#include "time_duration.h"

namespace utils
{
    void parse_address(const std::string& address, std::string& host, std::string& port, std::string& path, bool& use_ssl)
    {
        std::string_view tmp = address;

        auto pos = tmp.find("http://");
        if (pos != std::string::npos)
        {
            port.clear();
            port = "80";
            tmp.remove_prefix(7);
        }

        pos = tmp.find("https://");
        if (pos != std::string::npos)
        {
            port.clear();
            port = "443";
            tmp.remove_prefix(8);
            use_ssl = true;
        }

        pos = tmp.find(":");
        if (pos != std::string::npos)
        {
            port.clear();
            port = tmp.substr(pos + 1);
            tmp.remove_suffix(pos + 1);
        }

        pos = tmp.find("/");
        if (pos != std::string::npos)
        {
            path.clear();
            path = tmp.substr(pos);
            tmp.remove_suffix(pos + 1);
        }

        host = tmp;
    }

    // Timer
    Timer::~Timer() {
        stop();
    }
    
    void Timer::start(const Interval& interval, const Handler& handler, bool immediately /*= true*/) {
        m_handler = handler;
        m_interval = interval;
        if (immediately) {
            isStopped = false;
            run_once();
        } else {
            isStopped = true;
        }
    }
    
    void Timer::stop() {
        if (!isStopped) {
            std::unique_lock<std::mutex> guard(m_locker);
            isStopped = true;
            guard.unlock();
            cond.notify_all();
            if (m_thr.joinable()) {
                m_thr.join();
            }
        }
    }
    
    void Timer::run_once() {
        isStopped = false;
        m_thr = std::thread([&]() {
            std::unique_lock<std::mutex> guard(m_locker);
            if (cond.wait_for(guard, m_interval, [&](){return isStopped;})) {
                return;
            }
            guard.unlock();
            if (m_handler) {
                m_handler();
            }
        });
    }

    // time_duration
    time_duration::time_duration(bool _start):
        m_run(false)
    {
        if (_start)
            start();
    }

    time_duration::time_duration(bool _start, std::string message):
        m_run(false),
        m_msg(message)
    {
        if (_start)
            start();
    }

    time_duration::~time_duration()
    {
        stop();
    }

    void time_duration::start()
    {
        if (!m_run)
        {
            m_run = true;
            m_start = boost::posix_time::microsec_clock::local_time();
        }
    }

    void time_duration::stop()
    {
        if (m_run)
        {
            m_run = false;
            LOG_DBG("%s: %d millisec", m_msg.c_str(), (boost::posix_time::microsec_clock::local_time() - m_start).total_milliseconds())
        }
    }
}
