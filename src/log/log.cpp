#include <iostream>
#include "log.h"
#include <syslog.h>
#include <iostream>
#include <fstream>
#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;

namespace logg
{

IP7_Client* logger::_client = nullptr;
IP7_Trace*  logger::_trace = nullptr;

void init() {
    P7_Set_Crash_Handler();
    logger::_client = P7_Create_Client(TM("/P7.Sink=FileTxt /P7.Pool=1024 /P7.Format=\"[%tm] %ti %lv %ms\" /P7.Dir=./logs/ /P7.Roll=00:00tm"));
    if (logger::_client != nullptr) {
        logger::_trace = P7_Create_Trace(logger::_client, TM("channel"));
        if (logger::_trace) {
            logger::_trace->Share("main");
            //logger::_trace->Set_Verbosity(nullptr, EP7TRACE_LEVEL_COUNT);
        }
    }
}

void release() {
    if (logger::_trace) {
        logger::_trace->Release();
    }
    if (logger::_client) {
        logger::_client->Release();
    }
}

void push_err(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    IP7_Trace* tracer = P7_Get_Shared_Trace("main");
    if (tracer) {
        logg::logger::_trace->Trace_Embedded(0, EP7TRACE_LEVEL_ERROR, nullptr, 0, nullptr, nullptr, &format, &args);
        tracer->Release();
    }
    va_end(args);
}

void push_wrn(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    IP7_Trace* tracer = P7_Get_Shared_Trace("main");
    if (tracer) {
        logg::logger::_trace->Trace_Embedded(0, EP7TRACE_LEVEL_WARNING, nullptr, 0, nullptr, nullptr, &format, &args);
        tracer->Release();
    }
    va_end(args);
}

void push_inf(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    IP7_Trace* tracer = P7_Get_Shared_Trace("main");
    if (tracer) {
        logg::logger::_trace->Trace_Embedded(0, EP7TRACE_LEVEL_INFO, nullptr, 0, nullptr, nullptr, &format, &args);
        tracer->Release();
    }
    va_end(args);
}

void push_dbg(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    IP7_Trace* tracer = P7_Get_Shared_Trace("main");
    if (tracer) {
        logg::logger::_trace->Trace_Embedded(0, EP7TRACE_LEVEL_DEBUG, nullptr, 0, nullptr, nullptr, &format, &args);
        tracer->Release();
    }
    va_end(args);
}

}
