#include "fetch_balance_handler.h"

#include "settings/settings.h"
#include "data_storage/data_storage.h"
#include "data_storage/data_address.h"
#include "common/big_numbers.h"
#include "exception/except.h"

// fetch_balance_handler
fetch_balance_handler::fetch_balance_handler(http_session_ptr session):
    base_network_handler(settings::server::address, session),
    m_exec(!settings::service::local_data) {
    m_duration.set_message(__func__);
}

fetch_balance_handler::~fetch_balance_handler() {
}

bool fetch_balance_handler::prepare_params()
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
            CHK_PRM(storage::database::get_balance(addr, db_res), "failed on get balance from database")

            char buf[50] = {0};
            rapidjson::Value data_arr(rapidjson::kArrayType);
            if (!db_res.empty()) {
                json_rpc_reader reader;
                CHK_PRM(reader.parse(db_res.c_str()), "failed on parse balance from database");
                CHK_PRM(reader.get_doc().IsArray(), "balance from db has incorrect type");

                std::string_view address;
                if (reader.get_doc().Size() > 0) {
                    for (const auto& v: reader.get_doc().GetArray()) {
                        auto tmp = v.FindMember("address");
                        if (tmp == v.MemberEnd()) {
                            continue;
                        }
                        address = tmp->value.GetString();
                        common::BigFloat v1(0.0), v2(0.0);
                        tmp = v.FindMember("received");
                        if (tmp != v.MemberEnd()) {
                            v1.set_str(tmp->value.GetString(), 10);
                        }
                        tmp = v.FindMember("spent");
                        if (tmp != v.MemberEnd()) {
                            v2.set_str(tmp->value.GetString(), 10);
                        }
                        common::BigFloat v3 = (v1 - v2) / 1000000000000000000.0;
                        std::snprintf(buf, 49, "%.7f", v3.get_d());

                        rapidjson::Value obj(rapidjson::kObjectType);
                        obj.AddMember("address",
                                      rapidjson::Value(address.data(), static_cast<unsigned>(address.size()), m_writer.get_allocator()),
                                      m_writer.get_allocator());
                        obj.AddMember("balance", rapidjson::Value(buf, static_cast<unsigned>(strlen(buf)), m_writer.get_allocator()),
                                      m_writer.get_allocator());
                        obj.AddMember("currency", settings::service::coin_key, m_writer.get_allocator());
                        data_arr.PushBack(obj, m_writer.get_allocator());
                    }
                }
            }
            if (data_arr.Size() == 0) {
                rapidjson::Value obj(rapidjson::kObjectType);
                obj.AddMember("address",
                              rapidjson::Value(addr.data(), static_cast<unsigned>(addr.size()), m_writer.get_allocator()),
                              m_writer.get_allocator());
                obj.AddMember("balance", rapidjson::Value().SetString("0.0000000"),
                              m_writer.get_allocator());
                obj.AddMember("currency", settings::service::coin_key, m_writer.get_allocator());
                data_arr.PushBack(obj, m_writer.get_allocator());
            }
            m_writer.reset();
            m_writer.set_id(m_id);
            m_writer.set_result(rapidjson::Value("OK", m_writer.get_allocator()));
            m_writer.add("local", true);
            m_writer.add_value("data", data_arr);
            return true;
        }

        m_writer.add("method", "address.balance");
        m_writer.add("token", settings::service::token);

        params = m_writer.get_params();
        params->SetArray();

        rapidjson::Value obj(rapidjson::kObjectType);

        obj.AddMember("currency", settings::service::coin_key, m_writer.get_allocator());

        rapidjson::Value addr_arr(rapidjson::kArrayType);
        addr_arr.PushBack(rapidjson::Value(addr.c_str(), static_cast<unsigned>(addr.size())),
                          m_writer.get_allocator());

        obj.AddMember("address", addr_arr, m_writer.get_allocator());

        params->PushBack(obj, m_writer.get_allocator());

        return true;
    }
    END_TRY_RET(false)
}

void fetch_balance_handler::execute() {
    if (m_exec) {
        base_network_handler::execute();
    }
}

void fetch_balance_handler::execute(handler_callback callback) {
     base_network_handler::execute(callback);
}
