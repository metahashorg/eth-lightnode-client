#include "del_addresses_to_batch_tkn.h"
#include "exception/except.h"
#include "settings/settings.h"

del_addresses_to_batch_tkn::del_addresses_to_batch_tkn(http_session_ptr session)
    : base_network_handler(settings::server::address, session) {
    m_duration.set_message(__func__);
}

del_addresses_to_batch_tkn::~del_addresses_to_batch_tkn() {
}

bool del_addresses_to_batch_tkn::prepare_params()
{
    BGN_TRY
    {
        CHK_PRM(m_id, "id field not found")

        auto params = m_reader.get_params();
        CHK_PRM(params, "params field not found")

        m_writer.add("method", "tkn.batch.addresses.remove");
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
