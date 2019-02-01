#ifndef __FETCH_HISTORY_TKN_H__
#define __FETCH_HISTORY_TKN_H__

#include "network_handler.h"

class fetch_history_tkn : public base_network_handler {
public:
    fetch_history_tkn(http_session_ptr session);
    virtual ~fetch_history_tkn() override;
    
    virtual void execute() override;

protected:
    virtual bool prepare_params() override;

protected:
    bool m_exec = {true};
};

#endif // __FETCH_HISTORY_TKN_H__
