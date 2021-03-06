#include "add_to_tracking_handler.h"
#include "add_addresses_to_batch.h"
#include "add_addresses_to_batch_tkn.h"

#include "settings/settings.h"
#include "data_storage/data_address.h"
#include "data_storage/data_updater.h"
#include "exception/except.h"
#include "common/string_utils.h"

add_to_tracking_handler::add_to_tracking_handler(http_session_ptr session)
    : base_network_handler(settings::server::address, session) {
    m_duration.set_message(__FUNCTION__);
}

add_to_tracking_handler::~add_to_tracking_handler() {
}

bool add_to_tracking_handler::prepare_params()
{
    BGN_TRY
    {
        CHK_PRM(m_id, "id field not found")
        CHK_PRM(settings::service::local_data, "service not ran in tracking mode")

        auto params = m_reader.get_params();
        CHK_PRM(params, "params field not found")

        CHK_PRM(m_reader.get_value(*params, "address", m_address), "address field not found")
        CHK_PRM(!m_address.empty(), "address field is empty")
        CHK_PRM(m_address.compare(0, 2, "0x") == 0, "address field incorrect format")

        std::transform(m_address.begin(), m_address.end(), m_address.begin(), ::tolower);

        m_group = storage::addresses::group();
        CHK_PRM(!m_group.empty(), "group id was not defined");
        CHK_PRM(!storage::addresses::check(m_address), "address has added already");

        m_reader.get_value(*params, "reset", m_reset);

        return true;
    }
    END_TRY_RET(false)
}

void add_to_tracking_handler::execute()
{
    BGN_TRY
    {
        m_result.pending = true;

        std::string json = string_utils::str_concat("{\"id\":1, \"params\":{\"group\":\"", m_group, "\", \"address\":\"", m_address ,"\"}}");

        auto self = shared_from(this);
        auto result = perform<add_addresses_to_batch>(m_session, json,
            [self](const std::string_view& result) { self->on_batch_complete(result); });

        CHK_PRM(result.pending, "Failed on send 'add address to batch'");

        result = perform<add_addresses_to_batch_tkn>(m_session, json,
            [self](const std::string_view& result) { self->on_batch_tkn_complete(result); });

        CHK_PRM(result.pending, "Failed on send 'add address to batch token'");
    }
    END_TRY_PARAM(send_response())
}

void add_to_tracking_handler::execute(handler_callback)
{
    // TODO add if need
    CHK_PRM(false, "Not implement")
}

void add_to_tracking_handler::on_batch_complete(const std::string_view& param) {
    BGN_TRY
    {
        std::lock_guard<std::mutex> lock(m_locker);
        m_status[0] = status_code::scFalse;
        for (;;){
            if (param.empty()) {
                LOG_ERR("\"Add address to batch\" returns empty result");
                break;
            }
            json_rpc_reader reader;
            if (!reader.parse(param)) {
                LOG_ERR("Could not parse result from \"Add address to batch\". Json: %s", param.data());
                break;
            }
            auto tmp = reader.get_error();
            if (tmp) {
                LOG_ERR("Recieved error from \"Add address to batch\". %s", reader.stringify(tmp).data());
                break;
            }
            tmp = reader.get_result();
            if (!tmp) {
                LOG_ERR("Result not found in response \"Add address to batch\"");
                break;
            }
            m_status[0] = status_code::scTrue;
            break;
        }
        on_complete();
    }
    END_TRY_PARAM(on_complete());
}

void add_to_tracking_handler::on_batch_tkn_complete(const std::string_view& param) {
    BGN_TRY
    {
        std::lock_guard<std::mutex> lock(m_locker);
        m_status[1] = status_code::scFalse;
        for (;;){
            if (param.empty()) {
                LOG_ERR("\"Add address to batch token\" returns empty result");
                break;
            }
            json_rpc_reader reader;
            if (!reader.parse(param.data())) {
                LOG_ERR("Could not parse result from \"Add address to batch token\". Json: %s", param.data());
                break;
            }
            auto tmp = reader.get_error();
            if (tmp) {
                LOG_ERR("Recieved error from \"Add address to batch token\". %s", reader.stringify(tmp).data());
                break;
            }
            tmp = reader.get_result();
            if (!tmp) {
                LOG_ERR("Result not found in response \"Add address to batch token\"");
                break;
            }
            m_status[1] = status_code::scTrue;
            break;
        }
        on_complete();
    }
    END_TRY_PARAM(on_complete());
}

void add_to_tracking_handler::on_complete() {
    BGN_TRY
    {
        if (m_status[0] == status_code::scUndefined ||
            m_status[1] == status_code::scUndefined) {
            return;
        }
        m_writer.reset();
        if (m_status[0] == status_code::scTrue &&
            m_status[1] == status_code::scTrue) {
            if (!storage::addresses::store(m_address, m_reset)) {
                m_writer.set_error(rapidjson::Value().SetString("Could not store address in the storage"));
            } else {
                m_writer.set_result(rapidjson::Value().SetString("OK"));
            }
        } else {
            m_writer.set_error(rapidjson::Value().SetString("Failed"));
            rapidjson::Value status;
            status.SetObject();
            status.AddMember("add-to-batch",
                             m_status[0] == status_code::scTrue ? true : false, m_writer.get_allocator());
            status.AddMember("add-to-batch-tkn",
                             m_status[1] == status_code::scTrue ? true : false, m_writer.get_allocator());
            m_writer.add_value("status", status);
        }
        send_response();
    }
    END_TRY
}
