#ifndef BATCH_BALANCE_TKN_H_
#define BATCH_BALANCE_TKN_H_

#include "network_handler.h"

class batch_balance_tkn : public base_network_handler
{
public:
    batch_balance_tkn(http_session_ptr session);
    virtual ~batch_balance_tkn() override;
    
    virtual bool prepare_params() override;
};

#endif // BATCH_BALANCE_TKN_H_
