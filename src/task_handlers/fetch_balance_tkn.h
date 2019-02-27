#ifndef __FETCH_BALANCE_TKN_H__
#define __FETCH_BALANCE_TKN_H__

#include "network_handler.h"

class fetch_balance_tkn : public base_network_handler {
public:
    fetch_balance_tkn(http_session_ptr session);
    virtual ~fetch_balance_tkn() override;

//    virtual void execute() override;

protected:
    virtual void process_response(json_rpc_id id, json_rpc_reader &reader);
    virtual bool prepare_params() override;

protected:
    bool m_exec = {true};
    std::string m_contract;
};

#endif // __FETCH_BALANCE_TKN_H__

