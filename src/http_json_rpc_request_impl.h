#ifndef __HTTP_JSON_RPC_REQUEST_IMPL_H__
#define __HTTP_JSON_RPC_REQUEST_IMPL_H__

//#include <string.h>

#define BOOST_ERROR_CODE_HEADER_ONLY
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/beast/http/parser.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/exception_ptr.hpp>

#include "json_rpc.h"
#include "task_handlers/time_duration.h"
#include "task_handlers/utils.h"

namespace       asio    = boost::asio;
namespace       ssl     = boost::asio::ssl;
namespace       ip      = boost::asio::ip;
namespace       beast   = boost::beast;
namespace       http    = boost::beast::http;
using           tcp     = boost::asio::ip::tcp;

using http_json_rpc_execute_callback = std::function<void()>;

class http_json_rpc_request;

class http_json_rpc_request_impl: public std::enable_shared_from_this<http_json_rpc_request_impl>
{
public:
    http_json_rpc_request_impl(const std::string& host, asio::io_context& execute_context);
    ~http_json_rpc_request_impl();

    void set_path(const std::string_view& path);
    void set_body(const std::string_view& body);

    void execute();
    void execute_async(http_json_rpc_execute_callback callback);

    std::string_view get_result();

protected:
    void on_resolve(const boost::system::error_code& e, tcp::resolver::results_type eps);
    void on_connect(const boost::system::error_code& ec, const tcp::endpoint& ep);
    void on_handshake(const boost::system::error_code& e);
    void on_write(const boost::system::error_code& e);
    void on_read(const boost::system::error_code& e);
    void on_request_timeout();
    void on_connect_timeout();

    bool error_handler(const boost::system::error_code& e, const char* msg);
    void perform_callback();
    bool verify_certificate(bool preverified, ssl::verify_context& ctx);
    inline bool is_ssl() const { return m_use_ssl; }

    http_json_rpc_request* get_owner();
    void close();

private:
    bool                                m_async;
    bool                                m_use_ssl;
    bool                                m_canceled;
    asio::io_context&                   m_io_ctx;
    tcp::socket                         m_socket;
    tcp::resolver                       m_resolver;
    utils::Timer                        m_timer;
    utils::Timer                        m_connect_timer;
    utils::time_duration                m_duration;
    http::request<http::string_body>    m_req { http::verb::post, "/", 11 };
    http::response_parser<http::string_body>   m_response;
    boost::beast::flat_buffer           m_buf { 1024*16 };
    json_rpc_writer                     m_result;
    http_json_rpc_execute_callback      m_callback;
    std::string                         m_host;
    ssl::context                        m_ssl_ctx;
    ssl::stream<tcp::socket>            m_ssl_socket;
    std::string                         m_id;
    std::mutex                          m_locker;
};


#define JRPC_BGN try

#define JRPC_END(ret) \
    catch (boost::exception& ex) { \
        LOGERR << __PRETTY_FUNCTION__ << "Json-rpc boost exception: " << boost::diagnostic_information(ex) << " at line " << __LINE__; \
        std::lock_guard<std::mutex> lock(m_locker);\
        m_result.set_error(-32603, "Json-rpc boost exception. Check log for extra information.");\
        close();\
        m_canceled = true;\
        return ret;\
    } catch (std::exception& ex) { \
        LOGERR << __PRETTY_FUNCTION__ << "Json-rpc std exception: " << ex.what() << " at line " << __LINE__; \
        std::lock_guard<std::mutex> lock(m_locker);\
        m_result.set_error(-32603, "Json-rpc std exception. Check log for extra information.");\
        close();\
        m_canceled = true;\
        return ret;\
    } catch (...) { \
        LOGERR << __PRETTY_FUNCTION__ << "Json-rpc unhandled exception at line " << __LINE__; \
        std::lock_guard<std::mutex> lock(m_locker);\
        m_result.set_error(-32603, "Json-rpc unhandled exception. Check log for extra information.");\
        close();\
        m_canceled = true;\
        return ret;\
    }

#endif // __HTTP_JSON_RPC_REQUEST_IMPL_H__
