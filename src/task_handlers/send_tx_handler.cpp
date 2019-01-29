#include "send_tx_handler.h"
#include "../http_json_rpc_request.h"

send_tx_handler::send_tx_handler(http_session_ptr session)
    : create_tx_base_handler(session) {
    m_duration.set_message(__func__);
}

send_tx_handler::~send_tx_handler() {
}
