#ifndef EXCEPTIONS_H_
#define EXCEPTIONS_H_

#include <string>
//#include <sstream>
#include "../log/log.h"
#include "../eth_wallet/TypedException.h"

class invalid_param
{
public:
    invalid_param(const std::string& message);
    ~invalid_param() {}

    std::string_view what() { return m_msg; }
    std::string_view where() { return m_where; }

protected:
    std::string m_msg;
    std::string m_where;
};

#define CHK_PRM_RET(condition, ret, ...) \
if (!(condition)) {\
    LOG_ERR(__VA_ARGS__)\
    ret;}

#define CHK_PRM_BOOL(condition, ...) \
if (!(condition)) {\
    LOG_ERR(__VA_ARGS__)\
    return false;}

#define CHK_PRM(condition, message) \
    if (!(condition)) {\
        throw invalid_param(message); }

#define BGN_TRY try

#define END_TRY_RET_PARAM(ret, param) \
    catch (TypedException& e)\
    {\
        LOG_ERR("TypedException \"%s\" (%s : %u)", e.description.c_str(), __FILE__, __LINE__)\
        this->m_writer.reset();\
        this->m_writer.set_error(-32669, e.description);\
        param;\
        return ret;\
    }\
    catch (invalid_param& ex)\
    {\
        LOG_ERR("InvalidParam \"%s\" (%s : %u)", ex.what().data(), __FILE__, __LINE__)\
        this->m_writer.reset();\
        this->m_writer.set_error(-32668, ex.what().data());\
        param;\
        return ret;\
    }\
    catch (std::exception& ex)\
    {\
        LOG_ERR("STD Exception \"%s\" (%s : %u)", ex.what(), __FILE__, __LINE__)\
        this->m_writer.reset();\
        this->m_writer.set_error(-32667, ex.what());\
        param;\
        return ret;\
    }\
    catch(...)\
    {\
        LOG_ERR("Unknown exception (%s : %u)", __FILE__, __LINE__)\
        this->m_writer.reset();\
        this->m_writer.set_error(-32666, "Unhandled exception");\
        param;\
        return ret;\
    }

#define END_TRY_RET(ret)\
    END_TRY_RET_PARAM(ret, )

#define END_TRY_PARAM(param)\
    END_TRY_RET_PARAM(,param)

#define END_TRY\
    END_TRY_RET()

#endif // EXCEPTIONS_H_
