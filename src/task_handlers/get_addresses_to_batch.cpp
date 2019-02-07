#include "get_addresses_to_batch.h"
#include "exception/except.h"
#include "settings/settings.h"

get_addresses_to_batch::get_addresses_to_batch(http_session_ptr session): base_network_handler(settings::server::address, session) {
    m_duration.set_message(__func__);
}

get_addresses_to_batch::~get_addresses_to_batch() {
}

bool get_addresses_to_batch::prepare_params()
{
    BGN_TRY
    {
        CHK_PRM(m_id, "id field not found")

        auto params = m_reader.get_params();
        CHK_PRM(params, "params field not found")
        
        std::string_view group;
        CHK_PRM(m_reader.get_value(*params, "group", group), "group field not found")
        CHK_PRM(!group.empty(), "group is empty")
        
        m_writer.add("method", "batch.addresses");
        m_writer.add("token", settings::service::token);

        params = m_writer.get_params();
        params->SetArray();

        rapidjson::Value obj(rapidjson::kObjectType);

        obj.AddMember("currency",
                      rapidjson::Value().SetInt(settings::service::coin_key),
                      m_writer.get_allocator());

        obj.AddMember("group",
                      rapidjson::Value().SetString(group.data(), static_cast<unsigned>(group.size()), m_writer.get_allocator()),
                      m_writer.get_allocator());

        params->PushBack(obj, m_writer.get_allocator());

        return true;
    }
    END_TRY_RET(false)
}
