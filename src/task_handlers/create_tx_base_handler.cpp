#include "../eth_wallet/EthWallet.h"
#include "create_tx_base_handler.h"
#include "http_json_rpc_request.h"
#include "wallet_storage/wallet_storage.h"
#include "settings/settings.h"
#include "http_session.h"
#include "exception/except.h"

#include <boost/bind.hpp>

create_tx_base_handler::create_tx_base_handler(http_session_ptr session)
    : base_network_handler(settings::server::address, session){
}

create_tx_base_handler::~create_tx_base_handler() {
}

bool create_tx_base_handler::prepare_params()
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
        
        auto jValue = m_reader.get("nonce", *params);
        if (jValue)
        {
            CHK_PRM(json_utils::val2hex(jValue, m_nonce), "nonce field incorrect format")
        }
        
        jValue = m_reader.get("isPending", *params);
        if (jValue)
        {
            m_is_pending = jValue->GetString();
        }
        
        jValue = m_reader.get("fee", *params);
        CHK_PRM(jValue, "fee field not found")
        if (jValue->IsString())
        {
            std::string tmp = jValue->GetString();
            std::transform(tmp.begin(), tmp.end(), tmp.begin(), ::tolower);
            m_auto_fee = tmp.compare("auto") == 0;
        }
        if (!m_auto_fee)
        {
            std::string tmp;
            CHK_PRM(json_utils::val2str(jValue, tmp), "fee field incorrect format")
            m_fee.set_str(tmp, 10);
        }
        
        jValue = m_reader.get("value", *params);
        CHK_PRM(jValue, "value field not found")
        if (jValue->IsString())
        {
            std::string tmp = jValue->GetString();
            std::transform(tmp.begin(), tmp.end(), tmp.begin(), ::tolower);
            m_all_value = tmp.compare("all") == 0;
        }
        
        if (!m_all_value)
        {
            CHK_PRM(json_utils::val2hex(jValue, m_value), "value field incorrect format")
        }
        
        m_eth_wallet = std::make_unique<EthWallet>(settings::system::wallet_stotage, m_address, m_password);
        
        if (m_all_value)
        {
            get_balance();
            return false;
        }
        get_transaction_params();
        
        return false;
    }
    END_TRY_RET(false)
}
    
void create_tx_base_handler::build_request()
{
    BGN_TRY
    {
        if (!m_auto_fee)
        {
            std::string lim = m_gas_limit;
            if (lim.compare(0, 2, "0x") == 0)
            {
                lim.erase(0, 2);
            }
            mpz_class tmp(lim, 16);
            mpz_class res = m_fee / tmp;
            m_gas_price = res.get_str(16);
            m_gas_price.insert(0, "0x");
        }
        
        std::string data;
        const std::string transaction = m_eth_wallet->SignTransaction(m_nonce, m_gas_price, m_gas_limit, m_to, m_value, data);
        const std::string txHash = EthWallet::calcHash(transaction);
        
        m_writer.set_method("transaction.add.raw");
        m_writer.add("token", settings::service::token);
        auto params = m_writer.get_params();
        params->SetArray();
        
        rapidjson::Value obj(rapidjson::kObjectType);
        
        rapidjson::Value cur(rapidjson::kNumberType);
        cur.SetInt(settings::service::coin_key);
        obj.AddMember("currency", cur, m_writer.get_allocator());
        
        obj.AddMember("raw", transaction, m_writer.get_allocator());
        
        params->PushBack(obj, m_writer.get_allocator());
        
        boost::asio::post(boost::bind(&create_tx_base_handler::execute, shared_from(this), nullptr));
    }
    END_TRY_PARAM(boost::asio::post(boost::bind(&http_session::send_json, m_session, m_writer.stringify())))
}    

bool create_tx_base_handler::check_json(http_json_rpc_request_ptr request, json_rpc_id id)
{
    BGN_TRY
    {
        json_rpc_reader reader;
        CHK_PRM(reader.parse(request->get_result().c_str()), "Invalid response json")
        
        json_rpc_id _id = reader.get_id();
        CHK_PRM(_id != 0 && _id == id, "Returned id doesn't match")
        
        auto err = reader.get_error();
        auto res = reader.get_result();
        
        CHK_PRM(err || res, "No occur result or error")
        
        if (err)
        {
            m_writer.set_error(*err);
            return false;
        }
        
        return true;
    }
    END_TRY_RET(false)
}

void create_tx_base_handler::get_balance()
{
    //BGN_TRY
    {
        json_rpc_writer writer;
        json_rpc_id id = 1;
        writer.set_id(id);
        writer.add("method", "address.balance");
        writer.add("token", settings::service::token);
        
        auto params = writer.get_params();
        params->SetArray();
        
        rapidjson::Value obj(rapidjson::kObjectType);
        
        obj.AddMember("currency",
                        rapidjson::Value().SetInt(settings::service::coin_key),
                        writer.get_allocator());
        
        rapidjson::Value addr_arr(rapidjson::kArrayType);
        addr_arr.PushBack(rapidjson::Value().SetString(m_address, writer.get_allocator()),
                            writer.get_allocator());
        
        obj.AddMember("address", addr_arr, writer.get_allocator());
        
        params->PushBack(obj, writer.get_allocator());
        
        m_result.pending = true;
        auto request = std::make_shared<http_json_rpc_request>(settings::server::address, m_session->get_io_context());
        request->set_body(writer.stringify());
        request->execute_async(boost::bind(&create_tx_base_handler::on_get_balance, shared_from(this), request, id));
    }
    //END_TRY
}

void create_tx_base_handler::on_get_balance(http_json_rpc_request_ptr request, json_rpc_id id)
{
    BGN_TRY
    {
        if (!check_json(request, id))
        {
            boost::asio::post(boost::bind(&http_session::send_json, m_session, m_writer.stringify()));
            return;
        }
        
        json_rpc_reader reader;
        reader.parse(request->get_result().c_str());
        
        std::string balance;
        auto data = reader.get("data", reader.get_doc());
        CHK_PRM(data, "get balance: 'data' field not found")
        if (data->IsArray())
        {
            for (auto& v: data->GetArray())
            {
                std::string addr;
                if (!reader.get_value(v, "address", addr))
                    continue;
                std::transform(addr.begin(), addr.end(), addr.begin(), ::tolower);
                if (addr.compare(m_address) != 0)
                    continue;
                CHK_PRM(reader.get_value(v, "balance", balance), "get balance: 'balance' field not found")
                break;
            }
        }
        else if (data->IsObject())
        {
            CHK_PRM(reader.get_value(*data, "balance", balance), "get balance: 'balance' field not found")
        }
        else
        {
            CHK_PRM(false, "get balance: 'balance' field type not supported by service")
        }
        
        mpz_class mpz;
        mpz.set_str(balance, 10);
        mpz *= 1000000000000000000;
        
        m_value = mpz.get_str(16);
        m_value.insert(0, "0x");
        
        get_transaction_params();
        
        return;
    }
    END_TRY_PARAM(boost::asio::post(boost::bind(&http_session::send_json, m_session, m_writer.stringify())))
}

void create_tx_base_handler::get_transaction_params()
{
    //BGN_TRY
    {
        json_rpc_writer writer;
        json_rpc_id id = 1;
        writer.set_id(id);
        writer.add("method", "transaction.params");
        writer.add("token", settings::service::token);
        
        auto params = writer.get_params();
        params->SetArray();
        
        rapidjson::Value obj(rapidjson::kObjectType);
        
        rapidjson::Value cur(rapidjson::kNumberType);
        cur.SetInt(settings::service::coin_key);
        obj.AddMember("currency", cur, writer.get_allocator());
        
        rapidjson::Value addr(rapidjson::kStringType);
        addr.SetString(m_address, writer.get_allocator());
        obj.AddMember("address", addr, writer.get_allocator());
        
        if (!m_is_pending.empty()) {
            rapidjson::Value addr(rapidjson::kStringType);
            addr.SetString(m_is_pending, writer.get_allocator());
            obj.AddMember("isPending", addr, writer.get_allocator());
        }
        
        params->PushBack(obj, writer.get_allocator());
        
        m_result.pending = true;
        auto request = std::make_shared<http_json_rpc_request>(settings::server::address, m_session->get_io_context());
        request->set_body(writer.stringify());
        request->execute_async(boost::bind(&create_tx_base_handler::on_get_transaction_params, shared_from(this), request, id));
    }
    //END_TRY
}

void create_tx_base_handler::on_get_transaction_params(http_json_rpc_request_ptr request, json_rpc_id id)
{
    BGN_TRY
    {
        if (!check_json(request, id))
        {
            boost::asio::post(boost::bind(&http_session::send_json, m_session, m_writer.stringify()));
            return;
        }
        
        json_rpc_reader reader;
        CHK_PRM(reader.parse(request->get_result().c_str()), "get transaction params: remote service return invalid json")
        
        rapidjson::Document& doc = reader.get_doc();
        auto data = doc.FindMember("data");
        CHK_PRM(data != doc.MemberEnd(), "get transaction params: 'data' field not found")
        
        rapidjson::Value& data_val = data->value;
        CHK_PRM(data_val.IsObject(), "get transaction params: 'data' field incorrect")
        
        if (m_nonce.empty())
        {
            auto vnonce = data_val.FindMember("nonce");
            CHK_PRM(vnonce != data_val.MemberEnd(), "get transaction params: 'nonce' field not found")
            if (vnonce->value.IsString())
            {
                m_nonce = vnonce->value.GetString();
            }
        }
        
        if (m_gas_price.empty())
        {
            auto vgas_price = data_val.FindMember("gas_price");
            CHK_PRM(vgas_price != data_val.MemberEnd(), "get transaction params: 'gas_price' field not found")
            if (vgas_price->value.IsString())
            {
                m_gas_price = vgas_price->value.GetString();
            }
        }
        if (m_gas_price.empty())
        {
            LOG_WRN("gas price empty");
        }
        
        if (m_gas_limit.empty())
        {
            auto vgas_limit = data_val.FindMember("gas_limit");
            CHK_PRM(vgas_limit != data_val.MemberEnd(), "get transaction params: 'gas_limit' field not found")
            if (vgas_limit->value.IsString())
            {
                m_gas_limit = vgas_limit->value.GetString();
            }
        }
        if (m_gas_limit.empty())
        {
            LOG_WRN("gas limit empty");
        }
        
        build_request();
        
    }
    END_TRY_PARAM(boost::asio::post(boost::bind(&http_session::send_json, m_session, m_writer.stringify())))
}
