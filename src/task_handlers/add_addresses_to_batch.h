#ifndef ADD_ADDRESSES_TO_BATCH_H_
#define ADD_ADDRESSES_TO_BATCH_H_

#include "network_handler.h"

class add_addresses_to_batch : public base_network_handler
{
public:
    add_addresses_to_batch(http_session_ptr session);
    virtual ~add_addresses_to_batch() override;
    
    virtual bool prepare_params() override;
    
};

#endif // ADD_ADDRESSES_TO_BATCH_H_
