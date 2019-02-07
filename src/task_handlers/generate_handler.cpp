#include "generate_handler.h"
#include "eth_wallet/EthWallet.h"

#include "settings/settings.h"
#include "data_storage/data_address.h"
#include "http_json_rpc_request.h"
#include "common/string_utils.h"
#include "exception/except.h"

#include "add_to_tracking_handler.h"

// generate_handler
generate_handler::generate_handler(http_session_ptr session)
    : base_handler(session) {
    m_duration.set_message(__func__);
}

generate_handler::~generate_handler() {
}

bool generate_handler::prepare_params()
{
    BGN_TRY
    {
        CHK_PRM(m_id, "id field not found")

        auto params = m_reader.get_params();
        CHK_PRM(params, "params field not found")

        CHK_PRM(m_reader.get_value(*params, "password", m_password), "password field not found")
        CHK_PRM(!m_password.empty(), "password is empty")

        for (auto& c: m_password)
        {
            CHK_PRM(c >= 0x20 && c <= 0x7e, "password contains not allowed symbols")
        }

        return true;
    }
    END_TRY_RET(false)
}

void generate_handler::execute()
{
    BGN_TRY
    {
        std::string address = EthWallet::genPrivateKey(settings::system::wallet_stotage, m_password, settings::system::isLightKey);
        std::transform(address.begin(), address.end(), address.begin(), ::tolower);
        m_writer.add_result("address", address);
        if (settings::service::local_data) {
            perform<add_to_tracking_handler>(m_session, string_utils::str_concat("{\"id\":1, \"params\":{\"address\": \"", address, "\"}}"));
            m_result.pending = true;
        } else {
            storage::addresses::store(address);
        }
    }
    END_TRY
}

void generate_handler::execute(handler_callback) {
    BGN_TRY
    {
        CHK_PRM(false, "Not implemented")
    }
    END_TRY
}
