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

#include "connection_pool.h"

extern std::unique_ptr<socket_pool> g_conn_pool;

#define JRPC_BGN try

#define JRPC_END(ret) \
    catch (boost::exception& ex) { \
        LOG_ERR("%s Json-rpc boost exception: %s at line %d", __PRETTY_FUNCTION__, boost::diagnostic_information(ex).c_str(), __LINE__); \
        std::lock_guard<std::mutex> lock(m_locker);\
        m_result.set_error(-32603, "Json-rpc boost exception. Check log for extra information.");\
        close();\
        m_canceled = true;\
        ret;\
    } catch (std::exception& ex) { \
        LOG_ERR("%s Json-rpc std exception: %s at line %d", __PRETTY_FUNCTION__, ex.what(), __LINE__); \
        std::lock_guard<std::mutex> lock(m_locker);\
        m_result.set_error(-32603, "Json-rpc std exception. Check log for extra information.");\
        close();\
        m_canceled = true;\
        ret;\
    } catch (...) { \
        LOG_ERR("%s Json-rpc unhandled exception at line %d", __PRETTY_FUNCTION__, __LINE__); \
        std::lock_guard<std::mutex> lock(m_locker);\
        m_result.set_error(-32603, "Json-rpc unhandled exception. Check log for extra information.");\
        close();\
        m_canceled = true;\
        ret;\
    }

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
    m_ssl_socket(m_io_ctx, m_ssl_ctx)
{
    std::string addr, path, port;
    utils::parse_address(m_host, addr, port, path, m_use_ssl);

    m_req.version(11);
    m_req.set(http::field::host, addr);
    m_req.set(http::field::user_agent, "eth.service");
    m_req.set(http::field::content_type, "application/json");
//    m_req.set(http::field::keep_alive, false);
//    m_req.keep_alive(false);

    set_path(path);

    m_ssl_ctx.set_default_verify_paths();
    m_ssl_ctx.set_verify_mode(/*ssl::verify_fail_if_no_peer_cert*/ssl::verify_none);
    m_ssl_ctx.set_verify_callback([](bool, ssl::verify_context&){
        return true;
    });
}

http_json_rpc_request_impl::~http_json_rpc_request_impl()
{
    close();
}

void http_json_rpc_request_impl::set_path(const std::string_view& path)
{
    JRPC_BGN
    {
        if (path[0] != '/') {
            m_req.target(string_utils::str_concat("/", path));
        } else {
            m_req.target(path.data());
        }
    }
    JRPC_END()
}

void http_json_rpc_request_impl::set_body(const std::string_view& body)
{
    JRPC_BGN
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
    JRPC_END()
}

void http_json_rpc_request_impl::set_host(const std::string& host)
{
    JRPC_BGN
    {
        m_host = host;
        std::string addr, path, port;
        utils::parse_address(m_host, addr, port, path, m_use_ssl);
        m_req.set(http::field::host, addr);
    }
    JRPC_END()
}

bool http_json_rpc_request_impl::error_handler(const boost::system::error_code& e, const char* msg)
{
    JRPC_BGN
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

        LOG_ERR("Json-rpc[%s] Request error [%s] %d: %s", m_id.c_str(), msg, e.value(), e.message().c_str())

        close();

        //if (e != asio::error::operation_aborted)
        {
            m_result.set_error(-32603, string_utils::str_concat("Request error (", std::to_string(e.value()), "): ", e.message()).c_str());
            perform_callback();
        }

        if (!m_async && !m_io_ctx.stopped()) {
            m_io_ctx.stop();
        }

        m_duration.stop();

        return true;
    }
    JRPC_END(return true)
}

void http_json_rpc_request_impl::execute()
{
    JRPC_BGN
    {
        execute_async(nullptr);
        m_async = false;
        m_io_ctx.run();
    }
    JRPC_END()
}

void http_json_rpc_request_impl::execute_async(http_json_rpc_execute_callback callback)
{
    JRPC_BGN
    {
        m_duration.start();

        if (callback) {
            m_callback = boost::bind(callback);
        }

        m_result.reset();
        m_response.reset(new json_response_type());
        m_response->body_limit((std::numeric_limits<std::uint64_t>::max)());
        m_req.erase(http::field::connection);

        auto self = shared_from_this();
        bool need_connect = true;

        if (g_conn_pool->enable()) {
            m_pool_obj = g_conn_pool->checkout(m_host);
            if (!g_conn_pool->valid(m_pool_obj)) {
                m_req.set(http::field::connection, "close");
            } else if (m_pool_obj->socket != -1) {
                boost::system::error_code ec;
                tcp::endpoint ep;
                for (;;) {
                    if (is_ssl()) {
                        m_ssl_socket.lowest_layer().assign(tcp::v4(), m_pool_obj->socket, ec);
                        if (ec) break;
                        ep = m_ssl_socket.lowest_layer().remote_endpoint(ec);
                        if (ec) break;
                    } else {
                        m_socket.assign(tcp::v4(), m_pool_obj->socket, ec);
                        if (ec) break;
                        ep = m_socket.remote_endpoint(ec);
                        if (ec) break;
                    }
                    break;
                }

                if (!ec) {
                    need_connect = false;
                    if (is_ssl()) {
                        // does not works
                        http::async_write(m_ssl_socket, m_req, [self](const boost::system::error_code& e, std::size_t){
                            self->on_write(e);
                        });
                    } else {
                        http::async_write(m_socket, m_req, [self](const boost::system::error_code& e, std::size_t){
                            self->on_write(e);
                        });
                    }
                } else {
                    LOG_WRN("json-rpc[%s] Assign socket error: %s", m_id.c_str(), ec.message().c_str());
                    m_socket.close(ec);
                    m_ssl_socket.lowest_layer().close(ec);
                }
            }
        } else {
            m_req.set(http::field::connection, "close");
        }

        if (need_connect) {
            std::string addr, path, port;
            utils::parse_address(m_host, addr, port, path, m_use_ssl);

            m_resolver.async_resolve(addr, port, [self](const boost::system::error_code& e, tcp::resolver::results_type eps) {
                self->on_resolve(e, eps);
            });
        }
    }
    JRPC_END()
}

void http_json_rpc_request_impl::on_request_timeout()
{
    JRPC_BGN
    {
        std::lock_guard<std::mutex> lock(m_locker);
        if (m_canceled) {
            return;
        }

        m_canceled = true;

        LOG_ERR("Json-rpc[%s] Request timeout %u ms %u", m_id.c_str(), settings::system::jrpc_timeout);
        close();

        m_duration.stop();
        m_connect_timer.stop();
        m_timer.set_callback(nullptr);
        m_result.set_error(-32603, string_utils::str_concat("Request timeout ", std::to_string(settings::system::jrpc_timeout), " ms"));
        perform_callback();
    }
    JRPC_END()
}

void http_json_rpc_request_impl::on_resolve(const boost::system::error_code& e, tcp::resolver::results_type eps)
{
    JRPC_BGN
    {
        if (error_handler(e, __func__))
            return;

        auto self = shared_from_this();
        m_connect_timer.start(std::chrono::milliseconds(settings::system::jrpc_conn_timeout),[self](){
            self->on_connect_timeout();
        });

        asio::async_connect(is_ssl() ? m_ssl_socket.lowest_layer() : m_socket, eps, [self](const boost::system::error_code& e, const tcp::endpoint& ep){
            self->on_connect(e, ep);
        });
    }
    JRPC_END()
}

void http_json_rpc_request_impl::on_connect_timeout()
{
    JRPC_BGN
    {
        std::lock_guard<std::mutex> lock(m_locker);
        if (m_canceled) {
            return;
        }

        m_canceled = true;

        LOG_ERR("Json-rpc[%s] Connection to %s timeout %u ms", m_id.c_str(), m_host.c_str(), settings::system::jrpc_conn_timeout);

        close(true);

        m_duration.stop();
        m_connect_timer.set_callback(nullptr);
        m_timer.stop();
        m_result.set_error(-32603, string_utils::str_concat("Connection to ", m_host, " timeout ", std::to_string(settings::system::jrpc_conn_timeout), " ms"));
        perform_callback();
    }
    JRPC_END()
}

void http_json_rpc_request_impl::on_connect(const boost::system::error_code& e, const tcp::endpoint& ep)
{
    JRPC_BGN
    {
        if (error_handler(e, __func__)) {
            return;
        }

        m_connect_timer.stop();

        auto self = shared_from_this();
        m_timer.start(std::chrono::milliseconds(settings::system::jrpc_timeout), [self]() {
            self->on_request_timeout();
        });

        if (is_ssl()) {
            m_ssl_socket.async_handshake(ssl::stream<tcp::socket>::client, [self](const boost::system::error_code& e){
                self->on_handshake(e);
            });
        } else {
            //LOG_DBG("Json-rpc[%s] Send request: %s <<< %s", m_id.c_str(), m_host.c_str(), m_req.body().c_str())

            http::async_write(m_socket, m_req, [self](const boost::system::error_code& e, size_t /*sz*/){
                self->on_write(e);
            });
        }
    }
    JRPC_END()
}

void http_json_rpc_request_impl::on_handshake(const boost::system::error_code& e)
{
    JRPC_BGN
    {
        if (error_handler(e, __func__)) {
            return;
        }

        //LOG_DBG("Json-rpc[%s] Send request: %s <<< %s", m_id.c_str(), m_host.c_str(), m_req.body().c_str())

        auto self = shared_from_this();
        http::async_write(m_ssl_socket, m_req, [self](const boost::system::error_code& e, size_t /*sz*/) {
            self->on_write(e);
        });
    }
    JRPC_END()
}

void http_json_rpc_request_impl::on_write(const boost::system::error_code& e)
{
    JRPC_BGN
    {
        if (error_handler(e, __func__)) {
            return;
        }

        auto self = shared_from_this();
        if (is_ssl()) {
            http::async_read(m_ssl_socket, m_buf, *m_response, [self](const boost::system::error_code& e, size_t /*sz*/){
                self->on_read(e);
            });
        } else {
            http::async_read(m_socket, m_buf, *m_response, [self](const boost::system::error_code& e, size_t /*sz*/){
                self->on_read(e);
            });
        }
    }
    JRPC_END()
}

void http_json_rpc_request_impl::on_read(const boost::system::error_code& e)
{
    JRPC_BGN
    {
        if (error_handler(e, __func__)) {
            return;
        }

        m_timer.stop();

        http::status status = m_response->get().result();
        if (status != http::status::ok) {
            LOG_ERR("Json-rpc[%s] Incorrect response http status %d (%s)", m_id.c_str(), status, http::obsolete_reason(status).data())
        }

        const bool succ = m_result.parse(m_response->get().body().c_str());
        if (!succ) {
            LOG_ERR("Json-rpc[%s] Response json parse error: %u", m_id.c_str(), m_result.getDoc().GetParseError())
            if (status != http::status::ok) {
                m_result.set_error(static_cast<int>(status), "Incorrect response http status");
            }
        }

        //LOG_DBG("Json-rpc[%s] Recieve response: %s >>> %s", m_id.c_str(), m_host.c_str(), m_result.stringify().data())

        close();
        perform_callback();

        if (!m_async && !m_io_ctx.stopped()) {
            m_io_ctx.stop();
        }

        m_duration.stop();
    }
    JRPC_END()
}

void http_json_rpc_request_impl::perform_callback()
{
    JRPC_BGN
    {
        if (m_callback) {
            m_callback();
            m_callback = nullptr;
        }
    }
    JRPC_END()
}

std::string_view http_json_rpc_request_impl::get_result()
{
    return m_result.stringify();
}

//http_json_rpc_request* http_json_rpc_request_impl::get_owner()
//{
//    return nullptr;
//}

void http_json_rpc_request_impl::close(bool force)
{
    JRPC_BGN
    {
        boost::system::error_code ec;
        if (g_conn_pool->enable() && g_conn_pool->valid(m_pool_obj)) {
            if (force) {
                m_socket.shutdown(tcp::socket::shutdown_both, ec);
                m_socket.close(ec);
                m_ssl_socket.shutdown(ec);
                ec.clear();
            }
            if (is_ssl()) {
                if (m_ssl_socket.lowest_layer().native_handle() != -1) {
                    m_pool_obj->socket = m_ssl_socket.lowest_layer().release(ec);
                    if (ec) {
                        LOG_ERR("json-rpc[%s] release socket error: %s", m_id.c_str(), ec.message().c_str());
                    }
                } else {
                    m_pool_obj->socket = -1;
                }
                g_conn_pool->checkin(m_pool_obj);
                m_socket.shutdown(tcp::socket::shutdown_both, ec);
                m_socket.close(ec);
            } else {
                if (m_socket.native_handle() != -1) {
                    m_pool_obj->socket = m_socket.release(ec);
                    if (ec) {
                        LOG_ERR("json-rpc[%s] release socket error: %s", m_id.c_str(), ec.message().c_str());
                    }
                } else {
                    m_pool_obj->socket = -1;
                }
                g_conn_pool->checkin(m_pool_obj);
                m_ssl_socket.shutdown(ec);
            }
        } else {
            m_socket.shutdown(tcp::socket::shutdown_both, ec);
            m_socket.close(ec);
            m_ssl_socket.shutdown(ec);
        }
    }
    JRPC_END()
}

