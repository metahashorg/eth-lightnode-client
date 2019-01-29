#include "create_tx_token_handler.h"

#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include "http_session.h"

// create_tx_token_handler
create_tx_token_handler::create_tx_token_handler(http_session_ptr session)
    : create_tx_token_base_handler(session) {
     m_duration.set_message(__func__);
}

create_tx_token_handler::~create_tx_token_handler() {
}

void create_tx_token_handler::execute()
{
    // do nothing, just prepare
    this->m_writer.set_result(rapidjson::Value().SetString("ok"));
    boost::asio::post(boost::bind(&http_session::send_json, this->m_session, this->m_writer.stringify()));
    return;
}
