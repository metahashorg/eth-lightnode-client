#ifndef SEND_TX_HANDLER_H_
#define SEND_TX_HANDLER_H_

#include "create_tx_base_handler.h"

class send_tx_handler : public create_tx_base_handler
{
public:
    send_tx_handler(http_session_ptr session);
    virtual ~send_tx_handler() override;
};

#endif // SEND_TX_HANDLER_H_
