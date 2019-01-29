#include "del_addresses_to_batch.h"

#include "settings/settings.h"

#define RAPIDJSON_HAS_STDSTRING 1

bool del_addresses_to_batch::prepare_params()
{
    BGN_TRY
    {
        CHK_PRM(m_id, "id field not found")

        auto params = m_reader.get_params();
        CHK_PRM(params, "params field not found")

        m_writer.add("method", "batch.addresses.remove");
        m_writer.add("token", settings::service::token);

        std::string_view group;
        CHK_PRM(m_reader.get_value(*params, "group", group), "group field not found")
        CHK_PRM(!group.empty(), "group is empty")
        
        rapidjson::Value obj(rapidjson::kObjectType);

        obj.AddMember("currency",
                      rapidjson::Value().SetInt(settings::service::coin_key),
                      m_writer.get_allocator());

        auto data = params->FindMember("address");
        CHK_PRM(data != params->MemberEnd(), "get transaction params: 'address' field not found")
        
        rapidjson::Value& data_val = data->value;
        
        obj.AddMember("group",
                      rapidjson::Value().SetString(group.data(), static_cast<unsigned>(group.size()), m_writer.get_allocator()),
                      m_writer.get_allocator());
        obj.AddMember("address", data_val, m_writer.get_allocator());

        params = m_writer.get_params();
        params->SetArray();
        
        params->PushBack(obj, m_writer.get_allocator());

        return true;
    }
    END_TRY_RET(false)
}

del_addresses_to_batch::del_addresses_to_batch(http_session_ptr session)
    : base_network_handler(settings::server::address, session) {
    m_duration.set_message(__func__);
}

del_addresses_to_batch::~del_addresses_to_batch() {
}

