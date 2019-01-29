#ifndef BATCH_HISTORY_H_
#define BATCH_HISTORY_H_

#include "network_handler.h"

class batch_history : public base_network_handler
{
public:
    batch_history(http_session_ptr session);
    virtual ~batch_history() override;

protected:
    virtual bool prepare_params() override;
};

#endif // BATCH_HISTORY_H_
