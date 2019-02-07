#include "add_addresses_to_batch.h"
#include "exception/except.h"
#include "settings/settings.h"

#define RAPIDJSON_HAS_STDSTRING 1

add_addresses_to_batch::add_addresses_to_batch(http_session_ptr session)
    : base_network_handler(settings::server::address, session) {
    m_duration.set_message(__FUNCTION__);
}

add_addresses_to_batch::~add_addresses_to_batch() {
}

bool add_addresses_to_batch::prepare_params()
{
    BGN_TRY
    {
        CHK_PRM(m_id, "id field not found")

        auto params = m_reader.get_params();
        CHK_PRM(params, "params field not found")
        
        std::string_view group;
        CHK_PRM(m_reader.get_value(*params, "group", group), "group field not found")
        CHK_PRM(!group.empty(), "group is empty")
        
        auto data = params->FindMember("address");
        CHK_PRM(data != params->MemberEnd(), "get transaction params: 'address' field not found")

        m_writer.add("method", "batch.addresses.add");
        m_writer.add("token", settings::service::token);

        rapidjson::Value obj(rapidjson::kObjectType);

        obj.AddMember("currency",
                      rapidjson::Value().SetInt(settings::service::coin_key),
                      m_writer.get_allocator());
        
        rapidjson::Value& data_val = data->value;
        
        obj.AddMember("group",
                      rapidjson::Value().SetString(group.data(), static_cast<unsigned>(group.size()), m_writer.get_allocator())
                      , m_writer.get_allocator());
        obj.AddMember("address", data_val, m_writer.get_allocator());

        params = m_writer.get_params();
        params->SetArray();
        
        params->PushBack(obj, m_writer.get_allocator());

        return true;
    }
    END_TRY_RET(false)
}
