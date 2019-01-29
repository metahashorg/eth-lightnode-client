#ifndef DEL_ADDRESSES_TO_BATCH_TKN_H_
#define DEL_ADDRESSES_TO_BATCH_TKN_H_

#include "network_handler.h"

class del_addresses_to_batch_tkn : public base_network_handler {
public:
    
    del_addresses_to_batch_tkn(http_session_ptr session);
    virtual ~del_addresses_to_batch_tkn() override;

protected:
    virtual bool prepare_params() override;
};

#endif // DEL_ADDRESSES_TO_BATCH_TKN_H_
