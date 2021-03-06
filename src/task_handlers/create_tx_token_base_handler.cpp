#include "create_tx_token_base_handler.h"
#include "settings/settings.h"
#include "exception/except.h"
#include "eth_wallet/EthWallet.h"
#include "common/string_utils.h"

#include "task_handlers/fetch_balance_tkn.h"
#include "task_handlers/get_transaction_params_tkn.h"

create_tx_token_base_handler::create_tx_token_base_handler(http_session_ptr session)
    : base_network_handler(settings::server::address, session) {
}

create_tx_token_base_handler::~create_tx_token_base_handler() {
}

bool create_tx_token_base_handler::prepare_params()
{
    BGN_TRY
    {
        CHK_PRM(m_id, "id field not found")
        
        auto params = m_reader.get_params();
        CHK_PRM(params, "params field not found")
        
        CHK_PRM(m_reader.get_value(*params, "address", m_address) && !m_address.empty(), "address field not found")
        CHK_PRM(m_address.compare(0, 2, "0x") == 0, "address field must be in hex format")
        
        CHK_PRM(m_reader.get_value(*params, "password", m_password), "password field not found")
        CHK_PRM(!m_password.empty(), "empty password")
        
        CHK_PRM(m_reader.get_value(*params, "to", m_to) && !m_to.empty(), "to field not found")
        CHK_PRM(m_to.compare(0, 2, "0x") == 0, "to field must be in hex format")
        
        CHK_PRM(m_reader.get_value(*params, "token", m_token) && !m_token.empty(), "token field not found")
        CHK_PRM(m_token.compare(0, 2, "0x") == 0, "to field must be in hex format")
                
        auto jValue = m_reader.get("nonce", *params);
        if (jValue) {
            CHK_PRM(json_utils::val2hex(jValue, m_nonce), "nonce field incorrect format")
        }
        
        jValue = m_reader.get("fee", *params);
        CHK_PRM(jValue, "fee field not found")
        if (jValue->IsString()) {
            std::string tmp = jValue->GetString();
            std::transform(tmp.begin(), tmp.end(), tmp.begin(), ::tolower);
            m_auto_fee = tmp.compare("auto") == 0;
        }

        if (!m_auto_fee) {
            std::string tmp;
            CHK_PRM(json_utils::val2str(jValue, tmp), "fee field incorrect format")
            m_fee.set_str(tmp, 10);
        }

        jValue = m_reader.get("value", *params);
        CHK_PRM(jValue, "value field not found")
        if (jValue->IsString()) {
            std::string tmp = jValue->GetString();
            std::transform(tmp.begin(), tmp.end(), tmp.begin(), ::tolower);
            m_all_value = tmp.compare("all") == 0;
        }

        if (!m_all_value) {
            CHK_PRM(json_utils::val2hex(jValue, m_value), "value field incorrect format")
        }
        
        jValue = m_reader.get("isPending", *params);
        if (jValue) {
            m_is_pending = jValue->GetString();
        }

        m_eth_wallet = std::make_unique<EthWallet>(settings::system::wallet_stotage, m_address, m_password);
        return true;
    }
    END_TRY_RET(false)
}

void create_tx_token_base_handler::execute()
{
    BGN_TRY
    {
        m_result.pending = true;

        auto self = shared_from(this);
        handler_result res;

        if (m_all_value) {
            res = perform<fetch_balance_tkn>(m_session,
                string_utils::str_concat("{\"id\":1, \"params\":{\"address\":\"", m_address ,"\"}}"),
                [self](const std::string_view& result) { self->on_get_balance(result); });
            CHK_PRM(res.pending, "Failed on send 'fetch-balance-tkn'");
            return;
        }

        send_get_trans_params();
    }
    END_TRY_PARAM(send_response())
}

void create_tx_token_base_handler::execute(handler_callback)
{
    // TODO add if need
    CHK_PRM(false, "Not implement")
}

bool create_tx_token_base_handler::check_json(const std::string_view& result)
{
    BGN_TRY
    {
        json_rpc_reader reader;
        CHK_PRM(reader.parse(result), "Invalid response json")

        auto err = reader.get_error();
        auto res = reader.get_result();
        
        CHK_PRM(err || res, "No occur result or error")

        if (err) {
            m_writer.set_error(*err);
            return false;
        }

        return true;
    }
    END_TRY_RET(false)
}

void create_tx_token_base_handler::on_get_balance(const std::string_view& result)
{
    BGN_TRY
    {
        if (!check_json(result)) {
            send_response();
            return;
        }

        json_rpc_reader reader;
        reader.parse(result);

        auto data = reader.get("data", reader.get_doc());
        CHK_PRM(data, "get balance: 'data' field not found")
        CHK_PRM(data->IsArray(), "get balance: 'data' field type is not array")
        CHK_PRM(data->Size(), "get balance: no tokens found")
        for (auto& v: data->GetArray()) {
            std::string addr;
            if (!reader.get_value(v, "address", addr))
                continue;
            std::transform(addr.begin(), addr.end(), addr.begin(), ::tolower);
            if (addr.compare(m_address) != 0)
                continue;

            std::string ico_address;
            if (!reader.get_value(v, "ico_address", ico_address))
                continue;
            std::transform(ico_address.begin(), ico_address.end(), ico_address.begin(), ::tolower);
            if (ico_address.compare(m_token) != 0)
                continue;
                
            std::string received;
            CHK_PRM(reader.get_value(v, "received", received), "get received: 'received' field not found")
                
            std::string spent;
            CHK_PRM(reader.get_value(v, "spent", spent), "get received: 'spent' field not found")
                
            mpz_class mpzReceived;
            mpzReceived.set_str(received, 10);
            mpz_class mpzSpent;
            mpzSpent.set_str(spent, 10);
            mpz_class result = mpzReceived - mpzSpent;
            m_value = result.get_str(16);
            m_value.insert(0, "0x");

            break;
        }
        CHK_PRM(!m_value.empty(), "get balance: balance for token not found")

        send_get_trans_params();
    }
    END_TRY_PARAM(send_response())
}

void create_tx_token_base_handler::send_get_trans_params()
{
    BGN_TRY
    {
        auto self = shared_from(this);
        handler_result res;

        CHK_PRM(!m_value.empty(), "'value' is empty")

        std::string json = string_utils::str_concat("{\"id\":1, \"params\":{\"address\":\"", m_address ,"\",\"to_address\":\"", m_to ,"\",\"value\":\"");
        string_utils::str_append(json, m_value, "\",\"isPending\":\"", m_is_pending ,"\"}}");

        res = perform<get_transaction_params_tkn>(m_session, json,
            [self](const std::string_view& result) { self->on_get_trans_params(result); });
        CHK_PRM(res.pending, "Failed on send 'get-transaction-params-tkn'");

    }
    END_TRY_PARAM(send_response())
}

void create_tx_token_base_handler::on_get_trans_params(const std::string_view& result)
{
    BGN_TRY
    {
        if (!check_json(result)) {
            send_response();
            return;
        }

        json_rpc_reader reader;
        CHK_PRM(reader.parse(result), "get transaction params: remote service return invalid json")
        
        rapidjson::Document& doc = reader.get_doc();
        auto data = doc.FindMember("data");
        CHK_PRM(data != doc.MemberEnd(), "get transaction params: 'data' field not found")
        
        rapidjson::Value& data_val = data->value;
        CHK_PRM(data_val.IsObject(), "get transaction params: 'data' field incorrect")
        
        if (m_nonce.empty()) {
            auto vnonce = data_val.FindMember("nonce");
            CHK_PRM(vnonce != data_val.MemberEnd(), "get transaction params: 'nonce' field not found")
            if (vnonce->value.IsString()) {
                m_nonce = vnonce->value.GetString();
            }
        }
        
        if (m_gas_price.empty()) {
            auto vgas_price = data_val.FindMember("gas_price");
            CHK_PRM(vgas_price != data_val.MemberEnd(), "get transaction params: 'gas_price' field not found")
            if (vgas_price->value.IsString()) {
                m_gas_price = vgas_price->value.GetString();
            }
        }
        if (m_gas_price.empty()) {
            LOG_WRN("gas price is empty");
        }
        
        if (m_gas_limit.empty()) {
            auto vgas_limit = data_val.FindMember("gas_limit");
            CHK_PRM(vgas_limit != data_val.MemberEnd(), "get transaction params: 'gas_limit' field not found")
            if (vgas_limit->value.IsString()) {
                m_gas_limit = vgas_limit->value.GetString();
            }
        }
        auto erc20_data = data_val.FindMember("erc20_data");
        CHK_PRM(erc20_data != data_val.MemberEnd(), "get transaction params: 'erc20_data' field not found")
        CHK_PRM(erc20_data->value.IsString(), "get transaction params: incorrect 'erc20_data' value type")
        m_data = erc20_data->value.GetString();
        
        if (m_gas_limit.empty()) {
            LOG_WRN("'gas limit' is empty");
        }

        on_complete_job();
    }
    END_TRY_PARAM(send_response())
}

void create_tx_token_base_handler::on_complete_job()
{
    BGN_TRY
    {
        if (!m_auto_fee)
        {
            std::string_view lim = m_gas_limit;
            if (lim.compare(0, 2, "0x") == 0) {
                lim.remove_prefix(2);
            }
            mpz_class tmp(lim.data(), 16);
            mpz_class res = m_fee / tmp;
            m_gas_price = res.get_str(16);
            m_gas_price.insert(0, "0x");
        }

        const std::string transaction = m_eth_wallet->SignTransaction(m_nonce, m_gas_price, m_gas_limit, m_token, "0x00", m_data);
        const std::string txHash = EthWallet::calcHash(transaction);

        m_writer.set_method("tkn.transaction.add.raw");
        m_writer.add("token", settings::service::token);
        auto params = m_writer.get_params();
        params->SetArray();

        rapidjson::Value obj(rapidjson::kObjectType);

        rapidjson::Value cur(rapidjson::kNumberType);
        cur.SetInt(settings::service::coin_key);
        obj.AddMember("currency", cur, m_writer.get_allocator());

        obj.AddMember("raw", transaction, m_writer.get_allocator());

        params->PushBack(obj, m_writer.get_allocator());

        send_request();
    }
    END_TRY_PARAM(send_response())
}

void create_tx_token_base_handler::send_request()
{
    base_network_handler::execute();
}
