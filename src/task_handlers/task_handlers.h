#ifndef TASK_HANDLERS_H_
#define TASK_HANDLERS_H_

#include <memory>
#include <map>

#include "http_session_ptr.h"

struct handler_result;

using handler_func = handler_result(*)(http_session_ptr session, const std::string& params);

// TODO replace to string_view
extern const std::map<std::string, handler_func> post_handlers;

extern const std::map<std::string_view, handler_func> get_handlers;

#endif // TASK_HANDLERS_H_
