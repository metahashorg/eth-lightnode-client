#include "fetch_history_handler.h"

#include "settings/settings.h"
#include "data_storage/data_storage.h"
#include "data_storage/data_address.h"
#include "common/string_utils.h"
#include "common/big_numbers.h"
#include "rapidjson/document.h"
#include "exception/except.h"

// fetch_history_handler
fetch_history_handler::fetch_history_handler(http_session_ptr session):
    base_network_handler(settings::server::address, session),
    m_exec(!settings::service::local_data) {
    m_duration.set_message(__func__);
}

fetch_history_handler::~fetch_history_handler() {
}

bool fetch_history_handler::prepare_params()
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

        bool local = false;
        if (m_reader.get_value(*params, "local", local)) {
            m_exec = local == false;
        }
        if (!m_exec) {
            m_exec = storage::addresses::check(addr) == false;
        }
        if (!m_exec) {
            CHK_PRM(storage::database::open(), "could not open database")
            std::string db_res;
            storage::database::get_history(addr, db_res);
            if (db_res.empty()) {
                db_res = "[]";
            }

            json_rpc_reader reader;
            CHK_PRM(reader.parse(db_res.c_str()), "could not parse db result")
            CHK_PRM(reader.get_doc().IsArray(), "history from db has incorrect type");

            char buf_for_number[50] = {0};
            rapidjson::Value::MemberIterator tmp;
            rapidjson::Value arr(rapidjson::Type::kArrayType);

            for (auto& v: reader.get_doc().GetArray()) {
                rapidjson::Value obj(rapidjson::Type::kObjectType);

                tmp = v.FindMember("transaction");
                CHK_PRM(tmp != v.MemberEnd(), "Could not find field 'transaction'");

                obj.AddMember("id",
                    rapidjson::Value(tmp->value.GetString(), tmp->value.GetStringLength(), m_writer.get_allocator()),
                    m_writer.get_allocator());

                obj.AddMember("transaction",
                    rapidjson::Value(tmp->value.GetString(), tmp->value.GetStringLength(), m_writer.get_allocator()),
                    m_writer.get_allocator());

                obj.AddMember("tid", 0, m_writer.get_allocator());

                tmp = v.FindMember("from");
                CHK_PRM(tmp != v.MemberEnd(), "Could not find field 'from'");
                obj.AddMember("from_account", tmp->value,  m_writer.get_allocator());

                tmp = v.FindMember("to");
                CHK_PRM(tmp != v.MemberEnd(), "Could not find field 'to'");
                obj.AddMember("to_account", tmp->value,  m_writer.get_allocator());

                tmp = v.FindMember("is_input");
                CHK_PRM(tmp != v.MemberEnd(), "Could not find field 'is_input'");
                if (tmp->value.GetBool()) {
                    tmp = v.FindMember("from_value");
                    CHK_PRM(tmp != v.MemberEnd(), "Could not find field 'from_value'");
                } else {
                    tmp = v.FindMember("to_value");
                    CHK_PRM(tmp != v.MemberEnd(), "Could not find field 'to_value'");
                }
                common::BigFloat v1(tmp->value.GetString());
                v1 /= 1000000000000000000.0;
                double amount = v1.get_d();
                memset(buf_for_number, 0, sizeof(buf_for_number));
                int size = std::snprintf(buf_for_number, sizeof(buf_for_number)-1, "%.7f", amount);
                CHK_PRM(size != -1, "Could not format amount value");
                obj.AddMember("amount",
                              rapidjson::Value(buf_for_number, static_cast<unsigned>(size), m_writer.get_allocator()),
                              m_writer.get_allocator());

                obj.AddMember("fees", 0, m_writer.get_allocator());
                obj.AddMember("data", "", m_writer.get_allocator());
                obj.AddMember("net", "prod", m_writer.get_allocator());
                obj.AddMember("currency", settings::service::coin_key, m_writer.get_allocator());

                tmp = v.FindMember("block_number");
                CHK_PRM(tmp != v.MemberEnd(), "Could not find field 'block_number'");
                obj.AddMember("block", tmp->value, m_writer.get_allocator());

                tmp = v.FindMember("timestamp");
                CHK_PRM(tmp != v.MemberEnd(), "Could not find field 'timestamp'");
                obj.AddMember("ts", tmp->value, m_writer.get_allocator());

                tmp = v.FindMember("confirmations");
                CHK_PRM(tmp != v.MemberEnd(), "Could not find field 'confirmations'");
                obj.AddMember("confirmations", tmp->value, m_writer.get_allocator());

                arr.PushBack(obj.Move(), m_writer.get_allocator());
            }
            m_writer.set_result(rapidjson::Value("OK"));
            m_writer.add("local", true);
            m_writer.add_value("data", arr);
            return true;
        }

        m_writer.add("method", "address.transaction");
        m_writer.add("token", settings::service::token);

        params = m_writer.get_params();
        params->SetArray();

        rapidjson::Value obj(rapidjson::kObjectType);
        obj.AddMember("currency", settings::service::coin_key, m_writer.get_allocator());
        obj.AddMember("address",
                      rapidjson::Value(addr.c_str(), static_cast<unsigned>(addr.size()), m_writer.get_allocator()),
                      m_writer.get_allocator());

        params->PushBack(obj, m_writer.get_allocator());

        return true;
    }
    END_TRY_RET(false)
}

void fetch_history_handler::execute() {
    if (m_exec) {
        base_network_handler::execute();
    }
}

