#ifndef FETCH_BALANCE_HANDLER_H_
#define FETCH_BALANCE_HANDLER_H_

#include "network_handler.h"

class fetch_balance_handler : public base_network_handler {
public:
    fetch_balance_handler(http_session_ptr session);
    virtual ~fetch_balance_handler() override;
    
    virtual void execute() override;

protected:
    virtual bool prepare_params() override;

protected:
    bool m_exec = {true};
};

#endif // FETCH_BALANCE_HANDLER_H_
