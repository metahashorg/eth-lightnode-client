#ifndef CREATE_TX_HANDLER_H_
#define CREATE_TX_HANDLER_H_

#include "create_tx_base_handler.h"

class create_tx_handler : public create_tx_base_handler
{
public:
    create_tx_handler(http_session_ptr session);
    virtual ~create_tx_handler() override;

    virtual void send_request() override;
};

#endif // CREATE_TX_HANDLER_H_
