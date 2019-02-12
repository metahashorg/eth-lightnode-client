#include "network_handler.h"

#include <memory>
#include "http_json_rpc_request.h"
#include "log/log.h"
#include "http_session.h"
#include "exception/except.h"

#define BOOST_ERROR_CODE_HEADER_ONLY
#include <boost/asio/io_context.hpp>
#include <boost/asio/impl/post.hpp>

// TODO try to rid from boost/bind
#include <boost/bind.hpp>

base_network_handler::base_network_handler(const std::string &host, http_session_ptr session)
    : base_handler(session) {
    if (session) {
        m_request = std::make_unique<http_json_rpc_request>(host, session->get_io_context());
    } else {
        m_io_ctx.reset(new boost::asio::io_context());
        m_request = std::make_unique<http_json_rpc_request>(host, *m_io_ctx.get());
    }
}

base_network_handler::~base_network_handler() {
}

void base_network_handler::execute()
{
    BGN_TRY
    {
        m_request->set_body(m_writer.stringify());

        if (m_session == nullptr) {
            m_async_execute = false;
        }

        m_result.pending = m_async_execute;
        if (!m_async_execute) {
            m_request->execute();
            m_writer.reset();
            m_writer.parse(m_request->get_result().c_str());
        } else {
            m_request->execute_async(boost::bind(&base_network_handler::on_complete, shared_from(this)));
        }
    }
    END_TRY
}

void base_network_handler::execute(handler_callback callback)
{
    BGN_TRY
    {
        CHK_PRM(callback, "callback has not been presented");
        m_request->set_body(m_writer.stringify());

        m_callback = callback;
        m_async_execute = true;

        m_request->execute_async(boost::bind(&base_network_handler::on_complete_clbk, shared_from(this)));
    }
    END_TRY
}

void base_network_handler::process_response(json_rpc_id, json_rpc_reader &reader) {
    //json_rpc_id _id = reader.get_id();
    //CHK_PRM(_id != 0 && _id == id, "Returned id doesn't match")

    auto err = reader.get_error();
    auto res = reader.get_result();

    CHK_PRM(err || res, "No occur result or error")

    if (err) {
        m_writer.set_error(*err);
    } else if (res) {
        m_writer.set_result(*res);
    }

    rapidjson::Document& doc = reader.get_doc();
    for (auto& m : doc.GetObject()) {
        std::string name = m.name.GetString();
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        if (std::find(json_rpc_service.begin(), json_rpc_service.end(), name) != json_rpc_service.end()) {
            continue;
        }
        m_writer.add_value(m.name.GetString(), m.value);
    }
}

void base_network_handler::on_complete()
{
    BGN_TRY
    {
        m_writer.reset();
        json_rpc_reader reader;

        CHK_PRM(reader.parse(m_request->get_result().c_str()), "Invalid response json")

        process_response(m_id, reader);

        send_response();
    }
    END_TRY_PARAM(send_response())
}

void base_network_handler::on_complete_clbk()
{
    BGN_TRY
    {
        m_writer.reset();
        json_rpc_reader reader;

        CHK_PRM(reader.parse(m_request->get_result().c_str()), "Invalid response json")

        process_response(m_id, reader);

        boost::asio::post(boost::bind(m_callback, m_writer.stringify()));
        m_callback = nullptr;
    }
    END_TRY_PARAM(
                boost::asio::post(boost::bind(m_callback, m_writer.stringify()));
                m_callback = nullptr;)
}

void base_network_handler::send_response()
{
    boost::asio::post(boost::bind(&http_session::send_json, m_session, m_writer.stringify()));
}
