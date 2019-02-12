#ifndef BASE_HANDLER_H_
#define BASE_HANDLER_H_

#include <memory>

#include "http_session_ptr.h"
#include "json_rpc.h"
#include "log/log.h"
#include "time_duration.h"
#include <bits/std_function.h>

using mh_count_t = uint64_t;

struct handler_result
{
    operator bool() const                 {return !pending;}
    operator const std::string&() const   {return message;}
    operator const char*() const          {return message.c_str();}
    std::string message;
    bool pending = { false };
};

using handler_callback = std::function<void(const std::string& param)>;

class base_handler: public std::enable_shared_from_this<base_handler> {
public:
    base_handler(http_session_ptr& session);
    virtual ~base_handler();

    bool prepare(const std::string& params);

    virtual void execute() = 0;
    virtual void execute(handler_callback callback) = 0;

    handler_result result();

protected:
    virtual bool prepare_params() = 0;

    template <typename T>
    std::shared_ptr<T> shared_from(T*) {
        return std::static_pointer_cast<T>(shared_from_this());
    }
    
protected:
    handler_result          m_result;
    json_rpc_reader         m_reader;
    json_rpc_writer         m_writer;
    http_session_ptr        m_session;
    utils::time_duration    m_duration;
    json_rpc_id             m_id = { 0 };
};

template <class T>
static handler_result perform(http_session_ptr session, const std::string& params) {
    try {
        std::shared_ptr<T> obj = std::make_shared<T>(session);
        if (obj->prepare(params)) {
            obj->execute();
        }
        return obj->result();
    } catch (std::exception& ex) {
        LOG_ERR("handler perform exception %s: %s", __PRETTY_FUNCTION__, ex.what())
        return handler_result();
    } catch (...) {
        LOG_ERR("handler perform unhandled exception %s", __PRETTY_FUNCTION__)
        return handler_result();
    }
}

template <class T>
static handler_result perform(http_session_ptr session, const std::string& params, handler_callback callback) {
    try {
        std::shared_ptr<T> obj = std::make_shared<T>(session);
        if (obj->prepare(params)) {
            obj->execute(callback);
        }
        return obj->result();
    } catch (std::exception& ex) {
        LOG_ERR("handler perform exception %s: %s", __PRETTY_FUNCTION__, ex.what())
        return handler_result();
    } catch (...) {
        LOG_ERR("handler perform unhandled exception %s", __PRETTY_FUNCTION__)
        return handler_result();
    }
}

#endif // BASE_HANDLER_H_
