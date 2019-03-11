#ifndef __SIGNAL_HANDLER_H__
#define __SIGNAL_HANDLER_H__

#include <signal.h>

namespace common {

void set_signal_handler(__sighandler_t handler);

}

#endif // __SIGNAL_HANDLER_H__
