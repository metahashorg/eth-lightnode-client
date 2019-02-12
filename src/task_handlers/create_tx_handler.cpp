#include "create_tx_handler.h"

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
    send_response();
    return;
}
