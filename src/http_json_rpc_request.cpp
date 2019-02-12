#include "http_json_rpc_request.h"
#include "http_json_rpc_request_impl.cpp"

http_json_rpc_request::http_json_rpc_request(const std::string& host, asio::io_context& execute_context)
{
    m_impl = std::make_shared<http_json_rpc_request_impl>(host, execute_context);
}

http_json_rpc_request::~http_json_rpc_request()
{
}

void http_json_rpc_request::set_path(const std::string& path)
{
    m_impl->set_path(path);
}

void http_json_rpc_request::set_body(const std::string& body)
{
    m_impl->set_body(body);
}

void http_json_rpc_request::execute()
{
    m_impl->execute();
}

void http_json_rpc_request::execute_async(http_json_rpc_execute_callback callback)
{
    m_impl->execute_async(callback);
}

std::string http_json_rpc_request::get_result()
{
    return m_impl->get_result();
}
