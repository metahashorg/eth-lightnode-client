#include "task_handlers.h"

#include "create_tx_handler.h"
#include "send_tx_handler.h"
#include "create_tx_token_handler.h"
#include "send_tx_token_handler.h"
#include "generate_handler.h"
#include "fetch_balance_handler.h"
#include "fetch_history_handler.h"
#include "add_addresses_to_batch.h"
#include "del_addresses_to_batch.h"
#include "get_addresses_to_batch.h"
#include "batch_balance.h"
#include "batch_history.h"
#include "add_addresses_to_batch_tkn.h"
#include "del_addresses_to_batch_tkn.h"
#include "get_addresses_to_batch_tkn.h"
#include "batch_balance_tkn.h"
#include "batch_history_tkn.h"
#include "add_to_tracking_handler.h"
#include "del_from_tracking_handler.h"
#include "get_tracking_handler.h"

const std::map<std::string, handler_func> map_handlers = {
    { "generate",                       perform<generate_handler> },
    { "create-tx",                      perform<create_tx_handler> },
    { "send-tx",                        perform<send_tx_handler> },
    { "create-tx-token",                perform<create_tx_token_handler> },
    { "send-tx-token",                  perform<send_tx_token_handler> },
    { "fetch-balance",                  perform<fetch_balance_handler> },
    { "fetch-history",                  perform<fetch_history_handler> },
    { "add-addresses-to-batch",         perform<add_addresses_to_batch> },
    { "del-addresses-to-batch",         perform<del_addresses_to_batch> },
    { "get-addresses-to-batch",         perform<get_addresses_to_batch> },
    { "batch-balance",                  perform<batch_balance> },
    { "batch-history",                  perform<batch_history> },
    { "add-addresses-to-batch-tkn",     perform<add_addresses_to_batch_tkn> },
    { "del-addresses-to-batch-tkn",     perform<del_addresses_to_batch_tkn> },
    { "get-addresses-to-batch-tkn",     perform<get_addresses_to_batch_tkn> },
    { "batch-balance-tkn",              perform<batch_balance_tkn> },
    { "batch-history-tkn",              perform<batch_history_tkn> },
    { "add-to-tracking",                perform<add_to_tracking_handler> },
    { "del-from-tracking",              perform<del_from_tracking_handler> },
    { "get-tracking",                   perform<get_tracking_handler> }
};
