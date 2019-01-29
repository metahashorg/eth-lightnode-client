#ifndef TASK_HANDLERS_H_
#define TASK_HANDLERS_H_

#include <memory>
#include <map>

#include "http_session_ptr.h"

struct handler_result;

using handler_func = handler_result(*)(http_session_ptr session, const std::string& params);

extern const std::map<std::string, handler_func> map_handlers;

#endif // TASK_HANDLERS_H_
