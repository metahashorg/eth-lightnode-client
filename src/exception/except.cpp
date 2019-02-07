#include "except.h"
#include "common/string_utils.h"

invalid_param::invalid_param(const std::string& message)
    : m_msg(message)
{
    string_utils::str_append(m_where, __PRETTY_FUNCTION__, " in file ", __FILE__, " at line ", std::to_string(__LINE__));
}
