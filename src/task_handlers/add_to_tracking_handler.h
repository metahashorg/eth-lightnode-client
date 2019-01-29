#ifndef _ADD_TO_TRACKING_HANDLER_H_
#define _ADD_TO_TRACKING_HANDLER_H_

#include <mutex>
#include "network_handler.h"

class add_to_tracking_handler : public base_network_handler
{
public:
    enum status_code {
        scUndefined = -1,
        scFalse,
        scTrue
    };

    add_to_tracking_handler(http_session_ptr session);
    virtual ~add_to_tracking_handler() override;

    virtual bool prepare_params() override;

protected:
    void on_batch_complete(const std::string& param);
    void on_batch_tkn_complete(const std::string& param);

    void on_complete();

private:
    std::mutex  m_locker;
    std::string m_address;
    status_code m_status[2] = {status_code::scUndefined};
};

#endif // _ADD_TO_TRACKING_HANDLER_H_
