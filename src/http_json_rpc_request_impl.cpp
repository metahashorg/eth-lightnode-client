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

const std::string_view http_json_rpc_request_impl::step_str[] = {
    "ready",
    "connecting",
    "handshaking",
    "writing",
    "reading",
    "complete"
};

http_json_rpc_request_impl::http_json_rpc_request_impl(const std::string& host, asio::io_context& execute_context):
    m_async(true),
    m_use_ssl(false),
    m_canceled(false),
    m_io_ctx(execute_context),
    m_socket(m_io_ctx),
    m_resolver(m_io_ctx),
    m_duration(false, "json rpc"),
    m_host(host),
    m_ssl_ctx(ssl::context::sslv23),
    m_ssl_socket(m_io_ctx, m_ssl_ctx),
    m_step(step::ready)
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
    m_ssl_ctx.set_verify_mode(/*ssl::verify_fail_if_no_peer_cert*/ssl::verify_none);
    m_ssl_ctx.set_verify_callback([](bool, ssl::verify_context&){
        return true;
    });
}

http_json_rpc_request_impl::~http_json_rpc_request_impl()
{
    boost::system::error_code ec;
    if (m_socket.is_open()) {
        m_socket.shutdown(tcp::socket::shutdown_both, ec);
        m_socket.close(ec);
    }
    if (m_ssl_socket.lowest_layer().is_open()){
//        m_ssl_socket.shutdown(ec);
        m_ssl_socket.lowest_layer().shutdown(tcp::socket::shutdown_both, ec);
        m_ssl_socket.lowest_layer().close(ec);
    }

}

void http_json_rpc_request_impl::set_path(const std::string_view& path)
{
    if (path[0] != '/') {
        m_req.target(string_utils::str_concat("/", path));
    } else {
        m_req.target(path.data());
    }
}

void http_json_rpc_request_impl::set_body(const std::string_view& body)
{
    m_req.body().clear();
    m_req.body().append(body);
    m_req.set(http::field::content_length, m_req.body().size());

    json_rpc_reader reader;
    if (reader.parse(body)) {
        m_result.set_id(reader.get_id());
        m_id = reader.get_method();
        m_duration.set_message(string_utils::str_concat("json-rpc[", m_id, "]").c_str());
    }
}

bool http_json_rpc_request_impl::error_handler(const boost::system::error_code& e)
{
    std::lock_guard<std::mutex> lock(m_locker);

    if (m_canceled) {
        return true;
    }

    if (!e) {
        return false;
    }

    m_canceled = true;

    m_timer.stop();
    m_connect_timer.stop();

    LOG_ERR("Json-rpc[%s] Request error [%s] %d: %s", m_id.c_str(), step_str[static_cast<size_t>(m_step)].data(), e.value(), e.message().c_str())

    boost::system::error_code ec;
    if (m_socket.is_open()) {
        m_socket.shutdown(tcp::socket::shutdown_both, ec);
        m_socket.close(ec);
    }
    m_ssl_socket.shutdown(ec);

    //if (e != asio::error::operation_aborted)
    {
        m_result.set_error(e.value(), string_utils::str_concat("Request error: ", e.message()).c_str());
        perform_callback();
    }

    if (!m_async && !m_io_ctx.stopped()) {
        m_io_ctx.stop();
    }

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

    if (callback) {
        m_callback = boost::bind(callback);
    }

    std::string addr, path, port;
    utils::parse_address(m_host, addr, port, path, m_use_ssl);
    auto self = shared_from_this();
    m_resolver.async_resolve(addr, port, [self](const boost::system::error_code& e, tcp::resolver::results_type eps){
        self->on_resolve(e, eps);
    });
}

void http_json_rpc_request_impl::on_request_timeout()
{
    std::lock_guard<std::mutex> lock(m_locker);
    if (m_canceled) {
        return;
    }

    m_canceled = true;


    LOG_ERR("Json-rpc[%s] Request timeout [%s] %u ms %u", m_id.c_str(), step_str[static_cast<size_t>(m_step)].data(), settings::system::jrpc_timeout);

    m_connect_timer.stop();
    m_timer.set_callback(nullptr);

    m_result.set_error(32001, string_utils::str_concat("Request timeout ", std::to_string(settings::system::jrpc_timeout), " ms"));
    perform_callback();

    boost::system::error_code ec;
    if (m_socket.is_open()) {
        m_socket.shutdown(tcp::socket::shutdown_both, ec);
        m_socket.close(ec);
    }
    if (m_ssl_socket.lowest_layer().is_open()){
//        m_ssl_socket.shutdown(ec);
        m_ssl_socket.lowest_layer().shutdown(tcp::socket::shutdown_both, ec);
        m_ssl_socket.lowest_layer().close(ec);
    }

    m_duration.stop();
}

void http_json_rpc_request_impl::on_resolve(const boost::system::error_code& e, tcp::resolver::results_type eps)
{
    if (error_handler(e))
        return;

    auto self = shared_from_this();
    m_connect_timer.start(std::chrono::milliseconds(settings::system::jrpc_conn_timeout),[self](){
        self->on_connect_timeout();
    });

    m_step = step::connecting;
    asio::async_connect(is_ssl() ? m_ssl_socket.lowest_layer() : m_socket, eps, [self](const boost::system::error_code& e, const tcp::endpoint& ep){
        self->on_connect(e, ep);
    });
}

void http_json_rpc_request_impl::on_connect_timeout()
{
    std::lock_guard<std::mutex> lock(m_locker);
    if (m_canceled) {
        return;
    }

    m_canceled = true;

    LOG_ERR("Json-rpc[%s] Connection to %s timeout %u ms", m_id.c_str(), m_host.c_str(), settings::system::jrpc_conn_timeout);

//    m_connect_timer.stop();
    m_connect_timer.set_callback(nullptr);
    m_timer.stop();

    m_result.set_error(32002, string_utils::str_concat("Connection to ", m_host, " timeout ", std::to_string(settings::system::jrpc_conn_timeout), " ms"));
    perform_callback();

    boost::system::error_code ec;
    if (m_socket.is_open()) {
        m_socket.shutdown(tcp::socket::shutdown_both, ec);
        m_socket.close(ec);
    }
    if (m_ssl_socket.lowest_layer().is_open()){
//        m_ssl_socket.shutdown(ec);
        m_ssl_socket.lowest_layer().shutdown(tcp::socket::shutdown_both, ec);
        m_ssl_socket.lowest_layer().close(ec);
    }
    m_duration.stop();
}

void http_json_rpc_request_impl::on_connect(const boost::system::error_code& e, const tcp::endpoint& ep)
{
    if (error_handler(e)) {
        return;
    }

    m_connect_timer.stop();

    auto self = shared_from_this();
    m_timer.start(std::chrono::milliseconds(settings::system::jrpc_timeout), [self]() {
        self->on_request_timeout();
    });

    if (is_ssl()) {
        m_step = step::handshaking;
        m_ssl_socket.async_handshake(ssl::stream<tcp::socket>::client, [self](const boost::system::error_code& e){
            self->on_handshake(e);
        });
    } else {
        LOG_DBG("Json-rpc[%s] Send request: %s <<< %s", m_id.c_str(), m_host.c_str(), m_req.body().c_str())
        m_step = step::writing;
        http::async_write(m_socket, m_req, [self](const boost::system::error_code& e, size_t /*sz*/){
            self->on_write(e);
        });
    }
}

void http_json_rpc_request_impl::on_handshake(const boost::system::error_code& e)
{
    if (error_handler(e)) {
        return;
    }

    LOG_DBG("Json-rpc[%s] Send request: %s <<< %s", m_id.c_str(), m_host.c_str(), m_req.body().c_str())
    m_step = step::writing;
    auto self = shared_from_this();
    http::async_write(m_ssl_socket, m_req, [self](const boost::system::error_code& e, size_t /*sz*/) {
        self->on_write(e);
    });
}

void http_json_rpc_request_impl::on_write(const boost::system::error_code& e)
{
    if (error_handler(e)) {
        return;
    }

    m_step = step::reading;
    auto self = shared_from_this();
    if (is_ssl()) {
        http::async_read(m_ssl_socket, m_buf, m_response, [self](const boost::system::error_code& e, size_t /*sz*/){
            self->on_read(e);
        });
    } else {
        http::async_read(m_socket, m_buf, m_response, [self](const boost::system::error_code& e, size_t /*sz*/){
            self->on_read(e);
        });
    }
}

void http_json_rpc_request_impl::on_read(const boost::system::error_code& e)
{
    if (error_handler(e)) {
        return;
    }

    m_timer.stop();
    m_step = step::complete;

    http::status status = m_response.result();
    if (status != http::status::ok) {
        LOG_ERR("Json-rpc[%s] Incorrect response http status %d (%s)", m_id.c_str(), status, http::obsolete_reason(status).data())
    }

    const bool succ = m_result.parse(m_response.body().c_str());
    if (!succ) {
        LOG_ERR("Json-rpc[%s] Response json parse error: %u", m_id.c_str(), m_result.getDoc().GetParseError())
        if (status != http::status::ok) {
            m_result.set_error(static_cast<int>(status), "Incorrect response http status");
        }
    }

    LOG_DBG("Json-rpc[%s] Recieve response: %s >>> %s", m_id.c_str(), m_host.c_str(), m_result.stringify().data())

    perform_callback();

    if (!m_async && !m_io_ctx.stopped()) {
        m_io_ctx.stop();
    }

    m_duration.stop();
}

void http_json_rpc_request_impl::perform_callback()
{
    if (m_callback) {
        m_callback();
        m_callback = nullptr;
    }
}

std::string_view http_json_rpc_request_impl::get_result()
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
