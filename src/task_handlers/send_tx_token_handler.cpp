#include "send_tx_token_handler.h"
//#include "../http_json_rpc_request.h"

send_tx_token_handler::send_tx_token_handler(http_session_ptr session)
    : create_tx_token_base_handler(session) {
    m_duration.set_message(__func__);
}

send_tx_token_handler::~send_tx_token_handler() {
}
