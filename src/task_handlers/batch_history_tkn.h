#ifndef BATCH_HISTORY_TKN_H_
#define BATCH_HISTORY_TKN_H_

#include "network_handler.h"

class batch_history_tkn : public base_network_handler {
public:
    batch_history_tkn(http_session_ptr session);
    virtual ~batch_history_tkn() override;

protected:
    virtual bool prepare_params() override;
    
};

#endif // BATCH_HISTORY_TKN_H_
