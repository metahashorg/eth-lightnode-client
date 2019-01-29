#ifndef LOGG_H_
#define LOGG_H_

#include <string>
#include <sstream>
#include <map>
#include <mutex>
#include <thread>
#include "settings/settings.h"

#include "GTypes.h"
#include "P7_Trace.h"

#define USE_DEBUG_LOG_MESSAGE

namespace logg
{
    class logger
    {
    public:
        logger()                            = delete;
        ~logger()                           = delete;
        logger(const logger&)               = delete;
        logger& operator =(const logger&)   = delete;
        logger(const logger&&)              = delete;
        logger& operator =(const logger&&)  = delete;

        static IP7_Client* _client;
        static IP7_Trace*  _trace;
    };

    void init();
    void release();

    void push_err(const char* format, ...);
    void push_wrn(const char* format, ...);
    void push_inf(const char* format, ...);
    void push_dbg(const char* format, ...);
}

#define LOG_INF(...)\
    logg::push_inf(__VA_ARGS__);

#define LOG_ERR(...)\
    logg::push_err(__VA_ARGS__);

#define LOG_WRN(...)\
    logg::push_wrn(__VA_ARGS__);

#ifdef USE_DEBUG_LOG_MESSAGE
    #define LOG_DBG(...)\
        if (settings::system::debug_mode) {\
            logg::push_dbg(__VA_ARGS__); }
#else
    #define LOG_DBG(...)
#endif

#endif // LOGG_H_
