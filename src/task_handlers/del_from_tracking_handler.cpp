#include "del_from_tracking_handler.h"

#include "settings/settings.h"
#include "data_storage/data_address.h"
#include "common/string_utils.h"
#include "http_session.h"
#include "del_addresses_to_batch.h"
#include "del_addresses_to_batch_tkn.h"

#define BOOST_ERROR_CODE_HEADER_ONLY
#include <boost/bind/bind.hpp>

// del_from_tracking_handler
del_from_tracking_handler::del_from_tracking_handler(http_session_ptr session)
    : base_network_handler(settings::server::address, session) {
    m_duration.set_message(__func__);
}

del_from_tracking_handler::~del_from_tracking_handler() {
}

bool del_from_tracking_handler::prepare_params()
{
    BGN_TRY
    {
        CHK_PRM(m_id, "id field not found")
        CHK_PRM(settings::service::local_data, "service not ran in tracking mode")

        auto params = m_reader.get_params();
        CHK_PRM(params, "params field not found")

        CHK_PRM(m_reader.get_value(*params, "address", m_address), "address field not found")
        CHK_PRM(!m_address.empty(), "address field is empty")

        std::transform(m_address.begin(), m_address.end(), m_address.begin(), ::tolower);

        std::string_view group = storage::addresses::group();
        CHK_PRM(!group.empty(), "group id was not defined");
        CHK_PRM(storage::addresses::check(m_address), "address has been missing in tracking list");
        CHK_PRM(storage::addresses::remove(m_address), "could not remove address");

        std::string json = string_utils::str_concat("{\"id\":1, \"params\":{\"group\":\"", group, "\", \"address\":\"", m_address ,"\"}}");

        auto result = perform<del_addresses_to_batch>(m_session, json,
            boost::bind(&del_from_tracking_handler::on_batch_complete, shared_from(this), boost::placeholders::_1));

        CHK_PRM(result.pending, "Failed on send 'del address from batch'");

        result = perform<del_addresses_to_batch_tkn>(m_session, json,
                    boost::bind(&del_from_tracking_handler::on_batch_tkn_complete, shared_from(this), boost::placeholders::_1));

        CHK_PRM(result.pending, "Failed on send 'del address from batch token'");

        m_result.pending = true;
        return false;
    }
    END_TRY_RET(false)
}

void del_from_tracking_handler::on_batch_complete(const std::string& param) {
    BGN_TRY
    {
        std::lock_guard<std::mutex> lock(m_locker);
        m_status[0] = status_code::scFalse;
        for (;;){
            if (param.empty()) {
                LOG_ERR("\"Del address from batch\" returns empty result");
                break;
            }
            json_rpc_reader reader;
            if (!reader.parse(param.c_str())) {
                LOG_ERR("Could not parse result from \"Del address from batch\". Json: %s", param.c_str());
                break;
            }
            auto tmp = reader.get_error();
            if (tmp) {
                LOG_ERR("Recieved error from \"Del address from batch\". %s", reader.stringify(tmp).c_str());
                break;
            }
            tmp = reader.get_result();
            if (!tmp) {
                LOG_ERR("Result not found in response \"Del address from batch\"");
                break;
            }
            m_status[0] = status_code::scTrue;
            break;
        }
        on_complete();
    }
    END_TRY_PARAM(on_complete());
}

void del_from_tracking_handler::on_batch_tkn_complete(const std::string& param) {
    BGN_TRY
    {
        std::lock_guard<std::mutex> lock(m_locker);
        m_status[1] = status_code::scFalse;
        for (;;){
            if (param.empty()) {
                LOG_ERR("\"Del address from batch token\" returns empty result");
                break;
            }
            json_rpc_reader reader;
            if (!reader.parse(param.c_str())) {
                LOG_ERR("Could not parse result from \"Del address from batch token\". Json: %s", param.c_str());
                break;
            }
            auto tmp = reader.get_error();
            if (tmp) {
                LOG_ERR("Recieved error from \"Del address from batch token\". %s", reader.stringify(tmp).c_str());
                break;
            }
            tmp = reader.get_result();
            if (!tmp) {
                LOG_ERR("Result not found in response \"Del address from batch token\"");
                break;
            }
            m_status[1] = status_code::scTrue;
            break;
        }
        on_complete();
    }
    END_TRY_PARAM(on_complete());
}

void del_from_tracking_handler::on_complete() {
    BGN_TRY
    {
        if (m_status[0] == status_code::scUndefined ||
            m_status[1] == status_code::scUndefined) {
            return;
        }
        m_writer.reset();
        m_writer.set_result(rapidjson::Value().SetString("OK"));
        rapidjson::Value status;
        status.SetObject();
        status.AddMember("del-from-batch",
                         m_status[0] == status_code::scTrue ? true : false, m_writer.get_allocator());
        status.AddMember("del-from-batch-tkn",
                         m_status[1] == status_code::scTrue ? true : false, m_writer.get_allocator());
        m_writer.add_value("status", status);

        boost::asio::post(boost::bind(&http_session::send_json, m_session, m_writer.stringify()));
    }
    END_TRY
}
