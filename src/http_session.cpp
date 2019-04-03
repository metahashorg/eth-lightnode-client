#include "http_session.h"
#include "task_handlers/task_handlers.h"
#include "json_rpc.h"
#include "settings/settings.h"
#include "log/log.h"
#include "task_handlers/base_handler.h"
#include "common/string_utils.h"
#include "task_handlers/task_handlers.h"

#include <boost/beast/http.hpp>

http_session::http_session(tcp::socket&& socket) :
    m_socket(std::move(socket)),
    m_http_ver(11),
    m_http_keep_alive(false)
{
}

http_session::~http_session()
{
    close();
}

void http_session::run()
{
    m_req.clear();
    m_req.body().clear();
    m_buf.consume(m_buf.size());

    auto self = shared_from_this();
    http::async_read(m_socket, m_buf, m_req, [self](beast::error_code ec, std::size_t bytes_transferred)
    {
        if (!ec && bytes_transferred > 0) {
            self->process_request();
            if (self->keep_alive()) {
                self->run();
            }
        }
    });
}

asio::io_context& http_session::get_io_context()
{
    return m_socket.get_io_context();
}

void http_session::process_request()
{
    LOG_DBG("HTTP Session: %s >>> %s", m_socket.remote_endpoint().address().to_string().c_str(), m_req.body().c_str());

    for (;;) {
        m_http_ver = m_req.version();
        auto field = m_req.find(http::field::connection);
        if (field != m_req.end() && field->value() == "close") {
            m_http_keep_alive = false;
            break;
        }
        if (m_http_ver == 11) {
            m_http_keep_alive = true;
            break;
        }
        m_http_keep_alive = m_req.keep_alive();
        break;
    }

    switch(m_req.method()) {
    case http::verb::post:
        process_post_request();
        break;
    case http::verb::get:
        process_get_request();
        break;
    default:
        send_bad_request("Incorrect http method");
        break;
    }
}

void http_session::send_bad_request(const char* error)
{
    http::response<http::string_body> response;
    response.result(http::status::bad_request);
    response.set(http::field::content_type, "text/plain");
    //beast::ostream(response.body()) << error;
    response.body().append(error);
    send_response(response);
}

void http_session::send_json(const std::string_view& data)
{
    http::response<http::string_body> response;
    response.result(http::status::ok);
    response.set(http::field::content_type, "application/json");
    //beast::ostream(response.body()) << data.c_str();
    response.body().append(data);
    send_response(response);
}

void http_session::send_response(http::response<http::string_body>& response)
{
    LOG_DBG("HTTP Session: %s <<< %s", m_socket.remote_endpoint().address().to_string().c_str(), response.body().c_str());

    response.version(11);

    time_t dt = time(nullptr);
    struct tm tm = *gmtime(&dt);
    char buf[32] = {0};
    strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S %Z", &tm);

    response.set(http::field::date, buf);
    response.set(http::field::server, "eth.service");
    response.set(http::field::content_length, response.body().size());
    if (settings::service::keep_alive) {
        if (m_http_ver == 10) {
            response.set(http::field::connection, m_http_keep_alive ? "Keep-Alive" : "close");
        } else if (!m_http_keep_alive){
            response.set(http::field::connection, "close");
        }
    } else {
        response.set(http::field::connection, "close");
    }
    http::write(m_socket, response);
    if (!keep_alive()) {
        close();
    }
}

void http_session::process_post_request()
{
    if (m_req.target().size() != 1 || m_req.target()[0] != '/')
    {
        send_bad_request("Incorrect path");
        return;
    }

    std::string_view json;
    json_rpc_reader reader;
    json_rpc_writer writer;
    handler_result response;
    const std::string_view body(m_req.body().c_str(), m_req.body().size());
    if (reader.parse(body)) {
        auto it = post_handlers.find(reader.get_method());
        if (it == post_handlers.end()) {
            LOG_WRN("Incorrect service method %s", reader.get_method())

            writer.set_id(reader.get_id());
            writer.set_error(-32601, string_utils::str_concat("Method '", reader.get_method(), "' not found"));
            json = writer.stringify();
        } else {
            response = it->second(shared_from_this(), body);
            // async operation
            if (!response)
                return;
            json = response.message;
        }
    } else {
        LOG_ERR("Incorrect json %u: %s", reader.get_parse_error().Code(), m_req.body().c_str())
        writer.set_error(-32700, string_utils::str_concat("Json parse error: ", std::to_string(reader.get_parse_error().Code())));
        json = writer.stringify();
    }

    send_json(json);
}

void http_session::process_get_request()
{
    if (m_req.target().size() == 1) {
        send_bad_request("Incorrect path");
        return;
    }

    std::string_view params;
    std::string_view method(m_req.target().data(), m_req.target().size());
    size_t tmp = method.find_first_of('?');
    if (tmp != std::string_view::npos) {
        params = method.substr(tmp + 1, method.size() - tmp);
        method.remove_suffix(method.size() - tmp);
    }

    method.remove_prefix(1);

    std::string json;
    json_rpc_writer writer;
    handler_result response;
    auto it = get_handlers.find(method);
    if (it == get_handlers.end()) {
        LOG_WRN("Incorrect service method %s", method.data())
        writer.set_id(1);
        writer.set_error(-32601, string_utils::str_concat("Method '", method, "' not found"));
        json = writer.stringify();
    } else {
        writer.set_id(1);
        if (!params.empty()) {
            json_utils::to_json(params, *writer.get_params(), writer.get_allocator());
        }
        response = it->second(shared_from_this(), writer.stringify());
        // async operation
        if (!response)
            return;
        json.append(response.message);
    }
    send_json(json);
}

void http_session::close()
{
    boost::system::error_code ec;
    m_socket.shutdown(m_socket.shutdown_both, ec);
    m_socket.close(ec);
}

bool http_session::keep_alive()
{
    if (settings::service::keep_alive) {
        return m_http_keep_alive;
    }
    return false;
}
