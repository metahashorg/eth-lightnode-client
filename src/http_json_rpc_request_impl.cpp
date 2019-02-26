#include "http_json_rpc_request_impl.h"

#include "settings/settings.h"
#include "log/log.h"
#include "common/string_utils.h"

// TODO try to rid from boost/bind
#include <boost/bind.hpp>

#include <boost/asio/placeholders.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/connect.hpp>
#include <boost/beast/http/read.hpp>
#include <boost/beast/http/write.hpp>

http_json_rpc_request_impl::http_json_rpc_request_impl(const std::string& host, asio::io_context& execute_context):
    m_async(true),
    m_use_ssl(false),
    m_io_ctx(execute_context),
    m_socket(m_io_ctx),
    m_resolver(m_io_ctx),
    m_duration(false, "json rpc"),
    m_host(host),
    m_ssl_ctx(ssl::context::sslv23),
    m_ssl_socket(m_io_ctx, m_ssl_ctx)
{
    std::string addr, path, port;
    utils::parse_address(m_host, addr, port, path, m_use_ssl);
    m_req.set(http::field::host, addr);
    m_req.set(http::field::user_agent, "eth.service");
    m_req.set(http::field::content_type, "application/json");
    m_req.set(http::field::keep_alive, false);
    m_req.keep_alive(false);

    set_path(path);

    m_ssl_ctx.set_default_verify_paths();
    m_ssl_ctx.set_verify_mode(ssl::verify_fail_if_no_peer_cert);
    m_ssl_ctx.set_verify_callback(boost::bind(&http_json_rpc_request_impl::verify_certificate, this, _1, _2));
}

http_json_rpc_request_impl::~http_json_rpc_request_impl()
{
    boost::system::error_code ec;
    if (m_socket.is_open())
    {
        m_socket.shutdown(tcp::socket::shutdown_both, ec);
        m_socket.close(ec);
    }
    m_ssl_socket.shutdown(ec);
}

void http_json_rpc_request_impl::set_path(const std::string& path)
{
    if (path[0] != '/') {
        m_req.target(string_utils::str_concat("/", path));
    } else {
        m_req.target(path);
    }
}

void http_json_rpc_request_impl::set_body(const std::string& body)
{
    m_req.body().clear();
    m_req.body().append(body);
    //beast::ostream(m_req.body())
    //    << body.c_str();
    m_req.set(http::field::content_length, m_req.body().size());

    json_rpc_reader reader;
    if (reader.parse(body.c_str())) {
        m_result.set_id(reader.get_id());
        m_id = reader.get_method();
        m_duration.set_message(string_utils::str_concat("json-rpc[", m_id, "]").c_str());
    }
}

bool http_json_rpc_request_impl::error_handler(const boost::system::error_code& e)
{
    if (!e) {
        return false;
    }

    m_timer.stop();
    m_connect_timer.stop();

    LOG_ERR("Json-rpc[%s] Request error %d: %s", m_id.c_str(), e.value(), e.message().c_str())

    boost::system::error_code ec;
    if (m_socket.is_open())
    {
        m_socket.shutdown(tcp::socket::shutdown_both, ec);
        m_socket.close(ec);
    }

    m_ssl_socket.shutdown(ec);

    //if (e != asio::error::operation_aborted)
    {
        logg::push_err(e.message().c_str());
        m_result.set_error(32000, e.message().c_str());
        perform_callback();
    }

    if (!m_async && !m_io_ctx.stopped())
        m_io_ctx.stop();

    m_duration.stop();

    return true;
}

void http_json_rpc_request_impl::execute()
{
    execute_async(nullptr);
    m_async = false;
    m_io_ctx.run();
}

void http_json_rpc_request_impl::execute_async(http_json_rpc_execute_callback callback)
{
    m_duration.start();

    if (callback)
        m_callback = boost::bind(callback);

    std::string addr, path, port;
    utils::parse_address(m_host, addr, port, path, m_use_ssl);
    m_resolver.async_resolve(addr, port,
            boost::bind(&http_json_rpc_request_impl::on_resolve, shared_from_this(), asio::placeholders::error, asio::placeholders::results));
}

void http_json_rpc_request_impl::on_request_timeout()
{
    LOG_ERR("Json-rpc[%s] Request timeout %u millisec", m_id.c_str(), settings::system::jrpc_timeout)

    m_connect_timer.stop();

    m_result.set_error(32001, "Request timeout");
    perform_callback();
    boost::system::error_code ec;
    m_socket.close(ec);
    m_ssl_socket.shutdown(ec);
    m_duration.stop();
}

void http_json_rpc_request_impl::on_resolve(const boost::system::error_code& e, tcp::resolver::results_type eps)
{
    if (error_handler(e))
        return;

    m_connect_timer.start(std::chrono::milliseconds(settings::system::jrpc_conn_timeout),
                          boost::bind(&http_json_rpc_request_impl::on_connect_timeout, shared_from_this()));

    asio::async_connect(is_ssl() ? m_ssl_socket.lowest_layer() : m_socket, eps,
            boost::bind(&http_json_rpc_request_impl::on_connect, shared_from_this(), asio::placeholders::error, asio::placeholders::endpoint));
}

void http_json_rpc_request_impl::on_connect_timeout()
{
    LOG_ERR("Json-rpc[%s] Connect timeout %u millisec", m_id.c_str(), settings::system::jrpc_conn_timeout)

    boost::system::error_code ec;
    m_socket.cancel(ec);

    //m_connect_timer.run_once();
    m_connect_timer.stop();
}

void http_json_rpc_request_impl::on_connect(const boost::system::error_code& e, const tcp::endpoint&)
{
    m_connect_timer.stop();

    boost::system::error_code ec = e;
    if (ec.value() == ECANCELED)
    {
        ec.assign(ETIMEDOUT, ec.category());
    }

    if (error_handler(ec))
        return;

    m_timer.start(std::chrono::milliseconds(settings::system::jrpc_timeout),
                  boost::bind(&http_json_rpc_request_impl::on_request_timeout, shared_from_this()));

    if (is_ssl())
    {
        m_ssl_socket.async_handshake(ssl::stream<tcp::socket>::client,
            boost::bind(&http_json_rpc_request_impl::on_handshake, shared_from_this(), asio::placeholders::error));
    }
    else
    {
        LOG_DBG("Json-rpc[%s] Send request: %s <<< %s", m_id.c_str(), m_host.c_str(), m_req.body().c_str())

        http::async_write(m_socket, m_req,
            boost::bind(&http_json_rpc_request_impl::on_write, shared_from_this(), asio::placeholders::error));
    }
}

void http_json_rpc_request_impl::on_handshake(const boost::system::error_code& e)
{
    if (error_handler(e))
        return;

    LOG_DBG("Json-rpc[%s] Send request: %s <<< %s", m_id.c_str(), m_host.c_str(), m_req.body().c_str())

    http::async_write(m_ssl_socket, m_req,
        boost::bind(&http_json_rpc_request_impl::on_write, shared_from_this(), asio::placeholders::error));
}

void http_json_rpc_request_impl::on_write(const boost::system::error_code& e)
{
    if (error_handler(e))
        return;
    if (is_ssl())
    {
        http::async_read(m_ssl_socket, m_buf, m_response,
            boost::bind(&http_json_rpc_request_impl::on_read, shared_from_this(), asio::placeholders::error));
    }
    else
    {
        http::async_read(m_socket, m_buf, m_response,
            boost::bind(&http_json_rpc_request_impl::on_read, shared_from_this(), asio::placeholders::error));
    }
}

void http_json_rpc_request_impl::on_read(const boost::system::error_code& e)
{
    if (error_handler(e))
        return;

    m_timer.stop();

    http::status status = m_response.result();
    if (status != http::status::ok)
    {
        LOG_ERR("Json-rpc[%s] Incorrect response http status %d (%s)", m_id.c_str(), status, http::obsolete_reason(status).data())
    }

    const bool succ = m_result.parse(m_response.body().c_str());
    if (!succ)
    {
        LOG_ERR("Json-rpc[%s] Response json parse error: %u", m_id.c_str(), m_result.getDoc().GetParseError())
        if (status != http::status::ok) {
            m_result.set_error(static_cast<int>(status), "Incorrect response http status");
        }
    }

    LOG_DBG("Json-rpc[%s] Recieve response: %s >>> %s", m_id.c_str(), m_host.c_str(), m_result.stringify().c_str())

    perform_callback();

    if (!m_async && !m_io_ctx.stopped())
        m_io_ctx.stop();

    m_duration.stop();
}

void http_json_rpc_request_impl::perform_callback()
{
    if (m_callback)
    {
        m_callback();
        m_callback = nullptr;
    }
}

std::string http_json_rpc_request_impl::get_result()
{
    return m_result.stringify();
}

bool http_json_rpc_request_impl::verify_certificate(bool, ssl::verify_context&)
{
  return true;
}

http_json_rpc_request* http_json_rpc_request_impl::get_owner()
{
    return nullptr;
}
