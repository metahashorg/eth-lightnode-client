#ifndef __STATUS_HANDLER_H__
#define __STATUS_HANDLER_H__

#include "base_handler.h"

class status_handler : public base_handler {

    enum class cmd {
        general,
        keys
    };

public:
    status_handler(http_session_ptr session);
    virtual ~status_handler() override;
    
    virtual void execute() override;
    virtual void execute(handler_callback callback) override;

protected:
    virtual bool prepare_params() override;

private:
    cmd m_cmd = {cmd::general};
};

#endif // __STATUS_HANDLER_H__
