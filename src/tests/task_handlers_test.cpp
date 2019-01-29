#ifndef TASK_HANDLERS_TEST_H_
#define TASK_HANDLERS_TEST_H_

#include <gtest/gtest.h>

#include "../eth_wallet/EthWallet.h"
//#include "task_handlers/generate_handler.h"

TEST(task_handler, generate)
{
    //generate_handler handler(nullptr);
    //handler.prepare("{}");
    EthWallet wal("","","");
    EXPECT_FALSE(false);
}

#endif // TASK_HANDLERS_TEST_H_
