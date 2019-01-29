#ifndef NETWORK_HANDLER_H_
#define NETWORK_HANDLER_H_

#include "base_handler.h"
#include "http_json_rpc_request_ptr.h"

#define BOOST_ERROR_CODE_HEADER_ONLY
#include <boost/asio/io_context.hpp>

class base_network_handler : public base_handler
{
public:
    base_network_handler(const std::string& host, http_session_ptr session);
    virtual ~base_network_handler() override;

    virtual void execute() override;
    virtual void execute(handler_callback callback) override;

protected:

    // async callback
    void on_complete();
    void on_complete_clbk();

    virtual void process_response(json_rpc_id id, json_rpc_reader &reader);
    
protected:
    bool                        m_async_execute = {true};
    http_json_rpc_request_ptr   m_request;
    boost::asio::io_context     m_io_ctx;
    handler_callback            m_callback;
};

#endif // NETWORK_HANDLER_H_
