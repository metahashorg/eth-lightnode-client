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



        std::string_view group;
        CHK_PRM(m_reader.get_value(*params, "group", group), "group field not found")
        CHK_PRM(!group.empty(), "group is empty")

        std::string_view addr;
        CHK_PRM(m_reader.get_value(*params, "address", addr), "address field not found")
        CHK_PRM(!addr.empty(), "address is empty")
        CHK_PRM(addr.compare(0, 2, "0x") == 0, "address field incorrect format")

        m_writer.add("method", "tkn.batch.addresses.remove");
        m_writer.add("token", settings::service::token);

        rapidjson::Value obj(rapidjson::kObjectType);

        obj.AddMember("currency",
                      rapidjson::Value().SetInt(settings::service::coin_key),
                      m_writer.get_allocator());
        obj.AddMember("group",
                      rapidjson::Value().SetString(group.data(), static_cast<unsigned>(group.size()), m_writer.get_allocator()),
                      m_writer.get_allocator());
        obj.AddMember("address",
                      rapidjson::Value().SetString(addr.data(), static_cast<unsigned>(addr.size()), m_writer.get_allocator()),
                      m_writer.get_allocator());

        params = m_writer.get_params();
        params->SetArray();
        
        params->PushBack(obj, m_writer.get_allocator());

        return true;
    }
    END_TRY_RET(false)
}
