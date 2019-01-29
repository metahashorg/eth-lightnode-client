#ifndef GET_ADDRESSES_TO_BATCH_H_
#define GET_ADDRESSES_TO_BATCH_H_

#include "network_handler.h"

class get_addresses_to_batch : public base_network_handler {
public:
    get_addresses_to_batch(http_session_ptr session);
    virtual ~get_addresses_to_batch() override;

protected:
    virtual bool prepare_params() override;
};

#endif // GET_ADDRESSES_TO_BATCH_H_
