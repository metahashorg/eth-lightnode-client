#ifndef GENERATE_HANDLER_H_
#define GENERATE_HANDLER_H_

#include "base_handler.h"

class generate_handler : public base_handler {
public:
    generate_handler(http_session_ptr session);
    virtual ~generate_handler() override;
    
    virtual void execute() override;
    virtual void execute(handler_callback callback) override;

protected:
    virtual bool prepare_params() override;

private:
    std::string m_password;
};

#endif // GENERATE_HANDLER_H_
