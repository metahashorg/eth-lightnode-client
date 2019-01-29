#ifndef FETCH_HISTORY_HANDLER_H_
#define FETCH_HISTORY_HANDLER_H_

#include "network_handler.h"

class fetch_history_handler : public base_network_handler {
public:
    fetch_history_handler(http_session_ptr session);
    virtual ~fetch_history_handler() override;
    
    virtual void execute() override;

protected:
    virtual bool prepare_params() override;

protected:
    bool m_exec = {true};
};

#endif // FETCH_HISTORY_HANDLER_H_
