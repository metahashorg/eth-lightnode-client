#ifndef _GET_TRACKING_COUNT_HANDLER_H_
#define _GET_TRACKING_COUNT_HANDLER_H_

#include "base_handler.h"

class get_tracking_handler : public base_handler {
public:
    get_tracking_handler(http_session_ptr session);
    virtual ~get_tracking_handler() override;

    virtual void execute() override;
    virtual void execute(handler_callback callback) override;

protected:
    virtual bool prepare_params() override;

private:
    std::string m_address;
};

#endif // _GET_TRACKING_COUNT_HANDLER_H_
