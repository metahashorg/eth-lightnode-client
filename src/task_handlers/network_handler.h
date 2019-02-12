#ifndef NETWORK_HANDLER_H_
#define NETWORK_HANDLER_H_

#include "base_handler.h"
#include "http_json_rpc_request_ptr.h"
#include <memory>

//#define BOOST_ERROR_CODE_HEADER_ONLY
//#include <boost/asio/io_context.hpp>

namespace boost {
namespace asio {
class io_context;
}
}

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

    void send_response();
    
protected:
    bool                                        m_async_execute = {true};
    http_json_rpc_request_unique                m_request;
    std::unique_ptr<boost::asio::io_context>    m_io_ctx;
    handler_callback                            m_callback;
};

#endif // NETWORK_HANDLER_H_
