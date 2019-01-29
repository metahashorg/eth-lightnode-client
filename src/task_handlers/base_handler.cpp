#include "base_handler.h"

base_handler::base_handler(http_session_ptr& session)
    : m_session(session)
    , m_duration(false)
{}

base_handler::~base_handler() {
    m_duration.stop();
}

handler_result base_handler::result()
{
    m_result.message = m_writer.stringify();
    return m_result;
}

bool base_handler::prepare(const std::string& params)
{
    BGN_TRY
    {
        m_duration.start();

        CHK_PRM(this->m_reader.parse(params.c_str()), "Parse error");

        this->m_id = this->m_reader.get_id();
        this->m_writer.set_id(this->m_id);

        const bool complete = this->prepare_params();
        const bool pending = this->m_result.pending;
        if (!complete && !pending)
        {
            // prepare_params must set an error

            if (!this->m_writer.is_error())
            {
                this->m_writer.reset();
                this->m_writer.set_error(-32602, "Invalid params");
            }
        }

        LOG_DBG("Prepared json (%u,%u): %s", complete, pending, this->m_writer.stringify().c_str())

        return complete;
    }
    END_TRY_RET(false)
}
