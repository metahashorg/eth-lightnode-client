#include "signal_handler.h"

namespace common {

void set_signal_handler(__sighandler_t handler)
{
    // Termination Signals
    signal(SIGTERM, handler);
    signal(SIGINT, handler);
    signal(SIGQUIT, handler);
    signal(SIGKILL, handler);
    signal(SIGHUP, handler);

    // Program Error Signals
    signal(SIGFPE, handler);
    signal(SIGILL, handler);
    signal(SIGSEGV, handler);
    signal(SIGBUS, handler);
    signal(SIGABRT, handler);
    signal(SIGIOT, handler);
    signal(SIGTRAP, handler);
    signal(SIGSYS, handler);
}

}
