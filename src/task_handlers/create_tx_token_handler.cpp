#include "create_tx_token_handler.h"

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
    send_response();
    return;
}
