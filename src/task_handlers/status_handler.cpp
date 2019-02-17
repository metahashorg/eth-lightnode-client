#include "status_handler.h"
#include "settings/settings.h"
#include "exception/except.h"
#include "common/filesystem_utils.h"
#include <sys/types.h>
#include <dirent.h>

status_handler::status_handler(http_session_ptr session)
    : base_handler(session) {
    m_duration.set_message(__func__);
}

status_handler::~status_handler() {
}

bool status_handler::prepare_params()
{
    BGN_TRY
    {
        std::string_view cmd;
        if (auto params = m_reader.get_params()) {
            if (m_reader.get_value(*params, "cmd", cmd)) {
                if (cmd == "keys") {
                    m_cmd = cmd::keys;
                }
            }
        }
        return true;
    }
    END_TRY_RET(false)
}

void status_handler::execute()
{
    BGN_TRY
    {
        switch (m_cmd) {
            case cmd::general:
            m_writer.add_result("version", std::string("v0"));
            m_writer.add_result("server::address", settings::server::address);
            m_writer.add_result("service::local_data", settings::service::local_data);
            m_writer.add_result("service::coin_key", settings::service::coin_key);
            m_writer.add_result("service::gas_price_max", settings::service::gas_price_max.get_str());
            m_writer.add_result("service::gas_price_min", settings::service::gas_price_min.get_str());
            m_writer.add_result("service::data_storage", settings::system::data_storage);
            m_writer.add_result("service::wallet_stotage", settings::system::wallet_stotage);
            m_writer.add_result("service::debug_mode", settings::system::debug_mode);
            m_writer.add_result("service::jrpc_conn_timeout", settings::system::jrpc_conn_timeout);
            m_writer.add_result("service::jrpc_timeout", settings::system::jrpc_timeout);
            break;

        case cmd::keys:
        {
            rapidjson::Value arr(rapidjson::Type::kArrayType);
            if (fs_utils::dir::is_exists(settings::system::wallet_stotage.c_str())) {

                DIR* dirp = opendir(settings::system::wallet_stotage.c_str());
                struct dirent * dp;
                while ((dp = readdir(dirp)) != NULL) {
                    if (dp->d_type != DT_REG) {
                        continue;
                    }
                    arr.PushBack(rapidjson::Value().SetString(dp->d_name, static_cast<unsigned>(strlen(dp->d_name)), m_writer.get_allocator()),
                                 m_writer.get_allocator());
                }
                closedir(dirp);
            }
            m_writer.add_result("count", arr.Size());
            m_writer.get_value(m_writer.getDoc(), "result", rapidjson::Type::kObjectType).AddMember("keys", arr, m_writer.get_allocator());
            break;
        }
        default:
            m_writer.set_error(1, "unrecognized command");
            break;
        }

    }
    END_TRY
}

void status_handler::execute(handler_callback) {
    BGN_TRY
    {
        CHK_PRM(false, "Not implemented")
    }
    END_TRY
}
