#ifndef CREATE_TX_TOKEN_HANDLER_H_
#define CREATE_TX_TOKEN_HANDLER_H_

#include "create_tx_token_base_handler.h"

class create_tx_token_handler : public create_tx_token_base_handler
{
public:
    create_tx_token_handler(http_session_ptr session);
    virtual ~create_tx_token_handler() override;

    virtual void execute() override;
};

#endif // CREATE_TX_TOKEN_HANDLER_H_
