#ifndef __GET_TRANSACTION_PARAMS_H__
#define __GET_TRANSACTION_PARAMS_H__

#include "network_handler.h"

class get_transaction_params : public base_network_handler
{
public:
    get_transaction_params(http_session_ptr session);
    virtual ~get_transaction_params() override;

protected:
    virtual bool prepare_params() override;
};

#endif // __GET_TRANSACTION_PARAMS_H__

