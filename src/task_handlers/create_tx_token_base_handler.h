#ifndef CREATE_TX_TOKEN_BASE_HANDLER_H_
#define CREATE_TX_TOKEN_BASE_HANDLER_H_

#include "network_handler.h"
#include "http_session_ptr.h"
//#include "eth_wallet/EthWallet.h"
#include <gmpxx.h>
#include <mutex>

class EthWallet;

class create_tx_token_base_handler : public base_network_handler
{
    enum class job {
        balance,
        params
    };

    enum class job_status {
        undefined = -1,
        completed
    };

public:
    create_tx_token_base_handler(http_session_ptr session);
    virtual ~create_tx_token_base_handler() override;

    virtual void execute() override;
    virtual void execute(handler_callback callback) override;

protected:
    virtual bool prepare_params() override;
    virtual void send_request();

private:
    void on_get_trans_params(const std::string& result);
    void on_get_balance(const std::string& result);
    void on_complete_job();

    bool check_json(const std::string& result);
    
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
    job_status                  m_jobs[2] = {job_status::undefined, job_status::undefined};
    std::mutex                  m_locker;
};

#endif // CREATE_TX_TOKEN_BASE_HANDLER_H_
