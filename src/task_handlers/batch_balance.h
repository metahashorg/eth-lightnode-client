#ifndef BATCH_BALANCE_H_
#define BATCH_BALANCE_H_

#include "network_handler.h"

class batch_balance : public base_network_handler
{
public:
    batch_balance(http_session_ptr session);
    virtual ~batch_balance() override;
    
    virtual bool prepare_params() override;
};

#endif // BATCH_BALANCE_H_
