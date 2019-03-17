#ifndef _DEL_FROM_TRACKING_HANDLER_H_
#define _DEL_FROM_TRACKING_HANDLER_H_

#include "network_handler.h"
#include <mutex>

class del_from_tracking_handler : public base_network_handler
{
public:
    enum status_code {
        scUndefined = -1,
        scFalse,
        scTrue
    };

    del_from_tracking_handler(http_session_ptr session);
    virtual ~del_from_tracking_handler() override;

    virtual void execute() override;
    virtual void execute(handler_callback callback) override;

protected:
    virtual bool prepare_params() override;

    void on_batch_complete(const std::string_view& param);
    void on_batch_tkn_complete(const std::string_view& param);

    void on_complete();

private:
    std::mutex  m_locker;
    std::string m_address;
    status_code m_status[2] = {status_code::scUndefined};
    std::string_view m_group;
};

#endif // _DEL_FROM_TRACKING_HANDLER_H_
