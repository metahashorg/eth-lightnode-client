#include "get_transaction_params_tkn.h"

#include "settings/settings.h"
#include "exception/except.h"

get_transaction_params_tkn::get_transaction_params_tkn(http_session_ptr session):
    base_network_handler(settings::server::address, session)
{
    m_duration.set_message(__func__);
}

get_transaction_params_tkn::~get_transaction_params_tkn()
{
}

bool get_transaction_params_tkn::prepare_params()
{
    BGN_TRY
    {
        CHK_PRM(m_id, "id field not found")

        auto params = m_reader.get_params();
        CHK_PRM(params, "params field not found")

        std::string addr;
        CHK_PRM(m_reader.get_value(*params, "address", addr), "address field not found")
        CHK_PRM(!addr.empty(), "address is empty")
        CHK_PRM(addr.compare(0, 2, "0x") == 0, "address field incorrect format")

        std::transform(addr.begin(), addr.end(), addr.begin(), ::tolower);

        std::string to_addr;
        CHK_PRM(m_reader.get_value(*params, "to_address", to_addr), "to_address field not found")
        CHK_PRM(!to_addr.empty(), "to_address is empty")
        CHK_PRM(to_addr.compare(0, 2, "0x") == 0, "to_address field incorrect format")

        std::transform(to_addr.begin(), to_addr.end(), to_addr.begin(), ::tolower);

        std::string value;
        CHK_PRM(m_reader.get_value(*params, "value", value), "value field not found")
        const mpz_class result(value);
        value = "0x" + result.get_str(16);

        std::string pending;
        auto pend = m_reader.get("isPending", *params);
        if (pend) {
            pending = pend->GetString();
        }

        m_writer.add("method", "tkn.transaction.params");
        m_writer.add("token", settings::service::token);

        params = m_writer.get_params();
        params->SetArray();

        rapidjson::Value obj(rapidjson::kObjectType);

        obj.AddMember("currency", settings::service::coin_key, m_writer.get_allocator());

        rapidjson::Value addr_obj(rapidjson::kStringType);
        addr_obj.SetString(addr, m_writer.get_allocator());
        obj.AddMember("address", addr, m_writer.get_allocator());

        rapidjson::Value toaddr_obj(rapidjson::kStringType);
        addr_obj.SetString(to_addr, m_writer.get_allocator());
        obj.AddMember("to_address", to_addr, m_writer.get_allocator());

        rapidjson::Value value_obj(rapidjson::kStringType);
        value_obj.SetString(value, m_writer.get_allocator());
        obj.AddMember("value", value_obj, m_writer.get_allocator());

        if (!pending.empty()) {
            rapidjson::Value pend(rapidjson::kStringType);
            pend.SetString(pending, m_writer.get_allocator());
            obj.AddMember("isPending", pend, m_writer.get_allocator());
        }

        params->PushBack(obj, m_writer.get_allocator());
        return true;
    }
    END_TRY_RET(false)
}
