#include "create_tx_handler.h"

#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include "http_session.h"

// create_tx_handler
create_tx_handler::create_tx_handler(http_session_ptr session)
    : create_tx_base_handler(session) {
    m_duration.set_message(__func__);
}

create_tx_handler::~create_tx_handler() {
}

void create_tx_handler::execute()
{
    // do nothing, just prepare
    m_writer.set_result(rapidjson::Value().SetString("ok"));
    boost::asio::post(boost::bind(&http_session::send_json, m_session, m_writer.stringify()));
    return;
}
