#include "except.h"
#include "common/string_utils.h"

invalid_param::invalid_param(const std::string& message)
    : m_msg(message)
{
}
