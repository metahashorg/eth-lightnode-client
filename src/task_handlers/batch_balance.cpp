#include "batch_balance.h"
#include "exception/except.h"
#include "settings/settings.h"

#define RAPIDJSON_HAS_STDSTRING 1

batch_balance::batch_balance(http_session_ptr session): base_network_handler(settings::server::address, session) {
    m_duration.set_message(__FUNCTION__);
}

batch_balance::~batch_balance() {
}

bool batch_balance::prepare_params()
{
    BGN_TRY
    {
        CHK_PRM(m_id, "id field not found")

        auto params = m_reader.get_params();
        CHK_PRM(params, "params field not found")
        
        std::string_view group;
        CHK_PRM(m_reader.get_value(*params, "group", group), "group field not found")
        CHK_PRM(!group.empty(), "group is empty")
        
        uint64_t blockNum;
        CHK_PRM(m_reader.get_value(*params, "block", blockNum), "block field not found")
        
        m_writer.add("method", "batch.last.balance");
        m_writer.add("token", settings::service::token);

        rapidjson::Value obj(rapidjson::kObjectType);

        obj.AddMember("currency",
                      rapidjson::Value().SetInt(settings::service::coin_key),
                      m_writer.get_allocator());
        obj.AddMember("group",
                      rapidjson::Value().SetString(group.data(), static_cast<unsigned>(group.size()), m_writer.get_allocator()),
                      m_writer.get_allocator());
        obj.AddMember("block", blockNum, m_writer.get_allocator());

        params = m_writer.get_params();
        params->SetArray();
        
        params->PushBack(obj, m_writer.get_allocator());

        return true;
    }
    END_TRY_RET(false)
}
