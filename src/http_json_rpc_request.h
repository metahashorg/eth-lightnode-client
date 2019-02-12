#ifndef HTTP_JSON_RPC_REQUEST_H_
#define HTTP_JSON_RPC_REQUEST_H_

#include <string>
#include <memory>
#include <bits/std_function.h>

namespace boost {
namespace asio {
class io_context;
}
}

using http_json_rpc_execute_callback = std::function<void()>;

class http_json_rpc_request_impl;

class http_json_rpc_request
{
public:
    http_json_rpc_request(const std::string& host, boost::asio::io_context& execute_context);
    ~http_json_rpc_request();

    void set_path(const std::string& path);
    void set_body(const std::string& body);

    void execute();
    void execute_async(http_json_rpc_execute_callback callback);

    std::string get_result();

private:
    std::shared_ptr<http_json_rpc_request_impl> m_impl;
};

#endif // HTTP_JSON_RPC_REQUEST_H_
