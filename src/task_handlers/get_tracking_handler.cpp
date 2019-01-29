#include "get_tracking_handler.h"

#include "settings/settings.h"
#include "data_storage/data_address.h"

// get_tracking_handler
get_tracking_handler::get_tracking_handler(http_session_ptr session)
    : base_handler(session) {
    m_duration.set_message(__func__);
}

get_tracking_handler::~get_tracking_handler() {
}

bool get_tracking_handler::prepare_params()
{
    BGN_TRY
    {
        CHK_PRM(m_id, "id field not found")
        CHK_PRM(settings::service::local_data, "service not ran in tracking mode")
        std::string_view group = storage::addresses::group();
        CHK_PRM(!group.empty(), "group id was not defined");
        return true;
    }
    END_TRY_RET(false)
}

void get_tracking_handler::execute()
{
    BGN_TRY
    {
        std::string_view group = storage::addresses::group();

        auto params = m_writer.get_params();
        params->SetObject();

        params->AddMember("group",
                          rapidjson::Value().SetString(group.data(), static_cast<unsigned>(group.size()), m_writer.get_allocator()),
                          m_writer.get_allocator());

        rapidjson::Value arr(rapidjson::kArrayType);
        storage::addresses::storage_type list = storage::addresses::snapshot();
        for (const auto& addr: list) {
            arr.PushBack(rapidjson::Value().SetString(addr.c_str(), static_cast<unsigned>(addr.size()), m_writer.get_allocator()),
                         m_writer.get_allocator());
        }
        params->AddMember("addresses", arr, m_writer.get_allocator());
    }
    END_TRY
}

void get_tracking_handler::execute(handler_callback) {
    BGN_TRY
    {
        CHK_PRM(false, "Not implemented")
    }
    END_TRY
}
