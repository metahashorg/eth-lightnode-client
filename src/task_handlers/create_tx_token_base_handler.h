#ifndef CREATE_TX_TOKEN_BASE_HANDLER_H_
#define CREATE_TX_TOKEN_BASE_HANDLER_H_

#include "network_handler.h"
#include "http_session_ptr.h"
#include "eth_wallet/EthWallet.h"
#include <gmpxx.h>

class create_tx_token_base_handler : public base_network_handler
{
public:
    create_tx_token_base_handler(http_session_ptr session);
    virtual ~create_tx_token_base_handler() override;

protected:
    virtual bool prepare_params() override;

    bool check_params();
    void build_request();
    void on_get_transaction_params(http_json_rpc_request_ptr request, json_rpc_id id);
    void get_transaction_params();
    void on_get_balance(http_json_rpc_request_ptr request, json_rpc_id id);
    void get_balance();
    bool check_json(http_json_rpc_request_ptr request, json_rpc_id id);
    
protected:
    bool                        m_auto_fee = {false};
    bool                        m_all_value = {false};
    std::unique_ptr<EthWallet>  m_eth_wallet;
    std::string                 m_address;
    std::string                 m_password;
    std::string                 m_to;
    std::string                 m_token;
    std::string                 m_nonce;
    mpz_class                   m_fee;
    std::string                 m_value;
    std::string                 m_gas_price;
    std::string                 m_gas_limit;
    std::string                 m_data;
    std::string                 m_is_pending;
};

#endif // CREATE_TX_TOKEN_BASE_HANDLER_H_
