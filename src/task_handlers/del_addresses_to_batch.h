#ifndef DEL_ADDRESSES_TO_BATCH_H_
#define DEL_ADDRESSES_TO_BATCH_H_

#include "network_handler.h"

class del_addresses_to_batch : public base_network_handler{
public:
    del_addresses_to_batch(http_session_ptr session);
    virtual ~del_addresses_to_batch() override;

 protected:
    virtual bool prepare_params() override;
};

#endif // DEL_ADDRESSES_TO_BATCH_H_
