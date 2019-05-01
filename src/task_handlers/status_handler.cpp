#include "status_handler.h"
#include "settings/settings.h"
#include "exception/except.h"
#include "common/filesystem_utils.h"
#include <sys/types.h>
#include <dirent.h>
#include "connection_pool.h"

extern std::unique_ptr<socket_pool> g_conn_pool;

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
            m_writer.add_result("address", settings::server::address);
            m_writer.add_result("local_data", settings::service::local_data);
            m_writer.add_result("coin_key", settings::service::coin_key);
            m_writer.add_result("gas_price_max", settings::service::gas_price_max.get_str());
            m_writer.add_result("gas_price_min", settings::service::gas_price_min.get_str());
            m_writer.add_result("data_storage", settings::system::data_storage);
            m_writer.add_result("wallet_stotage", settings::system::wallet_stotage);
            m_writer.add_result("debug_mode", settings::system::debug_mode);
            m_writer.add_result("jrpc_conn_timeout (ms)", settings::system::jrpc_conn_timeout);
            m_writer.add_result("jrpc_timeout (ms)", settings::system::jrpc_timeout);
            m_writer.add_result("conn_pool_enable", g_conn_pool && g_conn_pool->enable() ? true : false);
            m_writer.add_result("conn_pool_ttl (sec)", settings::system::conn_pool_ttl);
            m_writer.add_result("conn_pool_capacity", settings::system::conn_pool_capacity);
            if (g_conn_pool && g_conn_pool->enable()) {
                m_writer.add_result("conn_pool_ready", g_conn_pool->get_ready_size());
                m_writer.add_result("conn_pool_busy", g_conn_pool->get_busy_size());
            }
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
