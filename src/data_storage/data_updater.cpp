#include "data_updater.h"
#include "data_address.h"
#include "data_storage.h"
#include "settings/settings.h"
#include "task_handlers/get_addresses_to_batch.h"
#include "task_handlers/del_addresses_to_batch.h"
#include "task_handlers/add_addresses_to_batch.h"
#include "task_handlers/get_addresses_to_batch_tkn.h"
#include "task_handlers/del_addresses_to_batch_tkn.h"
#include "task_handlers/add_addresses_to_batch_tkn.h"
#include "task_handlers/batch_balance.h"
#include "task_handlers/batch_history.h"
#include "task_handlers/batch_balance_tkn.h"
#include "task_handlers/batch_history_tkn.h"
#include "log/log.h"
#include "exception/except.h"
#include "common/string_utils.h"
#include <openssl/md5.h>

namespace storage
{
    std::unique_ptr<std::thread> updater::m_thr;
    bool updater::m_run = false;

    void updater::run() {
        m_run = true;
        m_thr = std::make_unique<std::thread>(updater::routine);
    }

    void updater::stop() {
        if (m_run) {
            m_run = false;
            m_thr->join();
        }
    }

    bool updater::prepare() {
        addresses::init();
        std::string_view group = addresses::group();
        CHK_PRM_BOOL(!group.empty(), "storage::updater::prepare: group must be defined")

        auto response = perform<get_addresses_to_batch>(nullptr, string_utils::str_concat("{\"id\":1, \"params\":{\"group\":\"", group, "\"}}"));
        CHK_PRM_BOOL(!response.message.empty(), "storage::updater::prepare: Response from batch.addresses is empty")

        rapidjson::Value
                *tmp = nullptr,
                *data = nullptr;

        json_rpc_reader reader;
        CHK_PRM_BOOL(reader.parse(response), "storage::updater::prepare: Parse result from batch.addresses error: %s", reader.get_parse_error().Code())

        tmp = reader.get_error();
        CHK_PRM_BOOL(!tmp, "storage::updater::prepare: Recieved error from batch.addresses. Error: %s", reader.stringify(tmp).c_str())

        tmp = reader.get_result();
        CHK_PRM_BOOL(tmp, "storage::updater::prepare: Could not recieve result from batch.addresses")

        data = reader.get("data", reader.get_doc());
        CHK_PRM_BOOL(tmp, "storage::updater::prepare: Data field not found in batch.addresses result")

        addresses::storage_type list = addresses::snapshot();

        std::vector<std::string> del;
        for (const auto& v: data->GetArray()) {
            auto it = std::find(list.begin(), list.end(), v.GetString());
            if (it == list.end()) {
                del.push_back(v.GetString());
            } else {
                list.erase(it);
            }
        }

        for (const auto& address: del) {
            response = perform<del_addresses_to_batch>(nullptr,
                string_utils::str_concat("{\"id\":1, \"params\":{\"address\": \"", address, "\", \"group\":\"", group, "\"}}"));

            CHK_PRM_BOOL(!response.message.empty(), "storage::updater::prepare: Response from batch.addresses.remove is empty")
            CHK_PRM_BOOL(reader.parse(response), "storage::updater::prepare: Parse result from batch.addresses.remove error: %s. Address: %s", reader.get_parse_error().Code(), address.c_str())

            tmp = reader.get_error();
            CHK_PRM_BOOL(!tmp, "storage::updater::prepare: Recieved error from batch.addresses.remove. Address: %s. Error: %s", address.c_str(), reader.stringify(tmp).c_str())

            tmp = reader.get_result();
            CHK_PRM_BOOL(tmp, "storage::updater::prepare: Could not recieve result from batch.addresses.remove. Address: %s", address.c_str())
        }

        for (const auto& address: list) {

            response = perform<add_addresses_to_batch>(nullptr,
                string_utils::str_concat("{\"id\":1, \"params\":{\"address\": \"", address, "\", \"group\":\"", group, "\"}}"));

            CHK_PRM_BOOL(!response.message.empty(), "storage::updater::prepare: Response from batch.addresses.add is empty")
            CHK_PRM_BOOL(reader.parse(response), "storage::updater::prepare: Parse result from batch.addresses.add error: %s. Address: %s", reader.get_parse_error().Code(), address.c_str())

            tmp = reader.get_error();
            CHK_PRM_BOOL(!tmp, "storage::updater::prepare: Recieved error from batch.addresses.add. Address: %s. Error: %s", address.c_str(), reader.stringify(tmp).c_str())

            tmp = reader.get_result();
            CHK_PRM_BOOL(tmp, "storage::updater::prepare: Could not recieve result from batch.addresses.add. Address: %s", address.c_str())
        }

        response = perform<get_addresses_to_batch_tkn>(nullptr, string_utils::str_concat("{\"id\":1, \"params\":{\"group\":\"", group, "\"}}"));
        CHK_PRM_BOOL(!response.message.empty(), "storage::updater::prepare: Response from tkn.batch.addresses is empty")

        CHK_PRM_BOOL(reader.parse(response), "storage::updater::prepare: Parse result from tkn.batch.addresses error: %s", reader.get_parse_error().Code())

        tmp = reader.get_error();
        CHK_PRM_BOOL(!tmp, "storage::updater::prepare: Recieved error from tkn.batch.addresses. Error: %s", reader.stringify(tmp).c_str())

        tmp = reader.get_result();
        CHK_PRM_BOOL(tmp, "storage::updater::prepare: Could not recieve result from tkn.batch.addresses")

        data = reader.get("data", reader.get_doc());
        CHK_PRM_BOOL(tmp, "storage::updater::prepare: Data field not found in batch.addresses result")

        list = addresses::snapshot();

        del.clear();
        for (const auto& v: data->GetArray()) {
            auto it = std::find(list.begin(), list.end(), v.GetString());
            if (it == list.end()) {
                del.push_back(v.GetString());
            } else {
                list.erase(it);
            }
        }

        for (const auto& address: del) {
            response = perform<del_addresses_to_batch_tkn>(nullptr,
                string_utils::str_concat("{\"id\":1, \"params\":{\"address\": \"", address, "\", \"group\":\"", group, "\"}}"));

            CHK_PRM_BOOL(!response.message.empty(), "storage::updater::prepare: Response from batch.addresses.remove is empty")
            CHK_PRM_BOOL(reader.parse(response), "storage::updater::prepare: Parse result from batch.addresses.remove error: %s. Address: %s", reader.get_parse_error().Code(), address.c_str())

            tmp = reader.get_error();
            CHK_PRM_BOOL(!tmp, "storage::updater::prepare: Recieved error from batch.addresses.remove. Address: %s. Error: %s", address.c_str(), reader.stringify(tmp).c_str())

            tmp = reader.get_result();
            CHK_PRM_BOOL(tmp, "storage::updater::prepare: Could not recieve result from batch.addresses.remove. Address: %s", address.c_str())
        }

        for (const auto& address: list) {
            response = perform<add_addresses_to_batch_tkn>(nullptr,
                string_utils::str_concat("{\"id\":1, \"params\":{\"address\": \"", address, "\", \"group\":\"", group, "\"}}"));

            CHK_PRM_BOOL(!response.message.empty(), "storage::updater::prepare: Response from batch.addresses.add is empty")
            CHK_PRM_BOOL(reader.parse(response), "storage::updater::prepare: Parse result from batch.addresses.add error: %s. Address: %s", reader.get_parse_error().Code(), address.c_str())

            tmp = reader.get_error();
            CHK_PRM_BOOL(!tmp, "storage::updater::prepare: Recieved error from batch.addresses.add. Address: %s. Error: %s", address.c_str(), reader.stringify(tmp).c_str())

            tmp = reader.get_result();
            CHK_PRM_BOOL(tmp, "storage::updater::prepare: Could not recieve result from batch.addresses.add. Address: %s", address.c_str())
        }

        LOG_INF("storage::updater::prepare: Successfully Done");
        return true;
    }

    void updater::routine() {
        try {
            if (!storage::database::open()) {
                LOG_ERR("storage::updater::routine: Could not open database");
                return;
            }

            if (!prepare()) {
                LOG_ERR("storage::updater::routine: Prepare failed");
                return;
            }

            std::string_view group = addresses::group();

            std::vector<std::string> addresses;
            std::chrono::system_clock::time_point tp;

            json_rpc_reader blns_reader, his_reader, db_reader;

            rapidjson::Value
                    *blnses = nullptr,
                    *value = nullptr,
                    *history = nullptr,
                    *tmp = nullptr;

            rapidjson::Value::MemberIterator m_it, addr_it;

            std::string db_blns, addr, his_addr, temp_str, hash_str;
            //std::string_view recvd, spent, db_recvd, db_spent;

            handler_result response;

            addresses::last_blocks blocks, tmp_blocks;

            unsigned char md5hash[MD5_DIGEST_LENGTH] = {0};
            char md5str[MD5_DIGEST_LENGTH*2+1] = {0};

            while(m_run){
                tp = std::chrono::high_resolution_clock::now() + std::chrono::seconds(1);

                blocks = addresses::get_last_blocks();

                response = perform<batch_balance>(nullptr,
                    string_utils::str_concat("{\"id\":1, \"params\":{\"block\": ", std::to_string(blocks.balance), ", \"group\":\"", group, "\"}}"));

                CHK_PRM_RET(!response.message.empty(), goto tokens,
                            "storage::updater::routine: Failed on get batch.last.balance for group \"%s\". Empty result", group.data());

                CHK_PRM_RET(blns_reader.parse(response), goto tokens,
                            "storage::updater::routine: Failed on parse batch.last.balance. Parse error: %u", blns_reader.get_parse_error().Code())

                tmp = blns_reader.get_error();
                CHK_PRM_RET(!tmp, goto tokens,
                            "storage::updater::routine: Failed on get batch.last.balance for group \"%s\". Error: %s", group.data(), blns_reader.stringify(tmp).c_str())

                tmp = blns_reader.get("data", blns_reader.get_doc());
                if (tmp == nullptr) {
                    LOG_ERR("storage::updater::routine: Could not get 'data' from batch.last.balance");
                    goto tokens;
                }
                if (!blns_reader.get_value(*tmp, "last_block", tmp_blocks.balance)) {
                    LOG_ERR("storage::updater::routine: Could not get 'last_block' from batch.last.balance");
                    goto tokens;
                }

                blnses = blns_reader.get("balances", *tmp);
                if (blnses == nullptr || !blnses->IsArray()) {
                    LOG_ERR("storage::updater::routine: Could not get 'balances' from batch.last.balance or wrong type");
                    goto tokens;
                }
                addresses.clear();
                for (auto& bl: blnses->GetArray()) {
                    addr.clear();
                    if (!blns_reader.get_value(bl, "address", addr)) {
                        LOG_ERR("storage::updater::routine: Could not get 'balances->address' from batch.last.balance");
                        goto tokens;
                    }
                    hash_str.clear();
                    if (!storage::database::get_hash_balance(addr, hash_str)) {
                        if (!storage::database::get_last_status().IsNotFound()) {
                            LOG_ERR("storage::updater::routine: Failed on get hash balance from db. Address: %s", addr.c_str());
                            goto tokens;
                        }
                    }
                    value = blns_reader.get("value", bl);
                    if (value == nullptr || !value->IsArray()) {
                        LOG_ERR("storage::updater::routine: Could not get 'balances->value' from batch.last.balance or wrong type");
                        goto tokens;
                    }

                    temp_str.clear();
                    temp_str = blns_reader.stringify(value);
                    MD5((const unsigned char*)temp_str.c_str(), temp_str.size(), md5hash);
                    string_utils::bin2hex(md5hash, sizeof(md5hash), md5str);

                    if (hash_str.compare(md5str) != 0) {
                        storage::database::set_balance(addr, temp_str, md5str);
                        addresses.emplace_back(addr);
                        LOG_INF("storage::updater::routine: Balance for \"%s\" was updated", addr.data());
                    }

//                    for (auto& v: value->GetArray()) {
//                        addr.clear();
//                        if (!blns_reader.get_value(v, "address", addr)) {
//                            LOG_ERR("storage::updater::routine: Could not get 'value->address' from batch.last.balance");
//                            continue;
//                        }
//                        std::transform(addr.begin(), addr.end(), addr.begin(), ::tolower);

//                        if (!blns_reader.get_value(v, "received", recvd)) {
//                            LOG_ERR("storage::updater::routine: Could not get 'value->received' from batch.last.balance");
//                            continue;
//                        }
//                        if (!blns_reader.get_value(v, "spent", spent)) {
//                            LOG_ERR("storage::updater::routine: Could not get 'value->spent' from batch.last.balance");
//                            continue;
//                        }
//                        db_blns.clear();
//                        if (!storage::database::get_balance(addr, db_blns)) {
//                            if (!storage::database::get_last_status().IsNotFound()) {
//                                LOG_ERR("storage::updater::routine: Failed on get balance from db. Address: %s", addr.c_str());
//                                continue;
//                            }
//                        }
//                        if (!db_blns.empty()) {
//                            if (!db_reader.parse(db_blns.c_str())) {
//                                LOG_ERR("storage::updater::routine: Failed on parse db balance. Parse error: %u. Json: %s", db_reader.get_parse_error().Code(), db_blns.c_str());
//                                continue;
//                            }
//                            if (!db_reader.get_value(db_reader.get_doc(), "received", db_recvd)) {
//                                LOG_ERR("storage::updater::routine: Could not found 'received' in db balance");
//                                continue;
//                            }
//                            if (!db_reader.get_value(db_reader.get_doc(), "spent", db_spent)) {
//                                LOG_ERR("storage::updater::routine: Could not found 'spent' in db balance");
//                                continue;
//                            }
//                            if (db_recvd.compare(recvd) == 0 && db_spent.compare(spent) == 0) {
//                                continue;
//                            }
//                        }

//                        storage::database::set_balance(addr, blns_reader.stringify(&v));
//                        addresses.emplace_back(addr);
//                        LOG_INF("storage::updater::routine: Balance for \"%s\" was updated", addr.data());
//                    }
                }
                if (!addresses.empty()) {

                    response = perform<batch_history>(nullptr,
                        string_utils::str_concat("{\"id\":1, \"params\":{\"block\": ", std::to_string(blocks.history), ", \"group\":\"", group, "\"}}"));

                    if (response.message.empty()) {
                        LOG_ERR("storage::updater::routine: Failed on get batch history for group \"%s\". Empty result.", group.data());
                        goto tokens;
                    }
                    if (!his_reader.parse(response)) {
                        LOG_ERR("storage::updater::routine: Failed on parse batch history. Parse error: %d", his_reader.get_parse_error().Code());
                        goto tokens;
                    }
                    tmp = his_reader.get_error();
                    if (tmp != nullptr) {
                        LOG_ERR("storage::updater::routine: Failed on get batch history for group \"%s\". Error: %s", group.data(), his_reader.stringify(tmp).c_str());
                        goto tokens;
                    }
                    tmp = his_reader.get("data", his_reader.get_doc());
                    if (tmp == nullptr) {
                        LOG_ERR("storage::updater::routine: Could not get 'data' from batch history");
                        goto tokens;
                    }
                    if (!his_reader.get_value(*tmp, "last_block", tmp_blocks.history)) {
                        LOG_ERR("storage::updater::routine: Could not get 'last_block' from batch history");
                        goto tokens;
                    }

                    history = his_reader.get("history", *tmp);
                    if (history == nullptr) {
                        LOG_ERR("storage::updater::routine: Could not get 'history' from batch history");
                        goto tokens;
                    }
                    for (const auto& addr: addresses) {
                        for (auto his_it = history->GetArray().Begin(); his_it != history->GetArray().End(); ++his_it) {
                            addr_it = his_it->FindMember("address");
                            if (addr_it == his_it->MemberEnd()) {
                                LOG_ERR("storage::updater::routine: Can not find ID field 'history->address' in batch history");
                                goto wait;
                            }

                            his_addr = addr_it->value.GetString();
                            std::transform(his_addr.begin(), his_addr.end(), his_addr.begin(), ::tolower);

                            if (his_addr.compare(addr) != 0) {
                                continue;
                            }
                            auto value = his_it->FindMember("value");
                            if (value == his_it->MemberEnd() && !value->value.IsArray()) {
                                LOG_ERR("storage::updater::routine: Can not find field 'history->value' in batch history. Address: %s", addr.c_str());
                                break;
                            }
                            if (value->value.Empty()) {
                                break;
                            }
                            temp_str.clear();
                            temp_str = his_reader.stringify(&value->value);
                            //string_utils::str_append(temp_str, "{\"data\":", his_reader.stringify(&value->value), "}");

                            MD5((const unsigned char*)temp_str.c_str(), temp_str.size(), md5hash);
                            string_utils::bin2hex(md5hash, sizeof(md5hash), md5str);

                            if (storage::database::get_hash_history(addr, hash_str)) {
                                if (hash_str.compare(md5str) == 0) {
                                    break;
                                }
                            }

                            storage::database::set_history(addr, temp_str, md5str);
                            LOG_INF("storage::updater::routine: History for \"%s\" was updated", addr.c_str());
                            break;
                        }
                    }
                }
tokens:
                // tokens
                response = perform<batch_balance_tkn>(nullptr,
                    string_utils::str_concat("{\"id\":1, \"params\":{\"block\": ", std::to_string(blocks.tkn_balance), ", \"group\":\"", group, "\"}}"));

                CHK_PRM_RET(!response.message.empty(), goto wait,
                            "storage::updater::routine: Failed on get tkn.batch.last.balance for group \"%s\". Empty result", group.data());

                CHK_PRM_RET(blns_reader.parse(response), goto wait,
                            "storage::updater::routine: Failed on parse tkn.batch.last.balance. Parse error: %u", blns_reader.get_parse_error().Code())

                tmp = blns_reader.get_error();
                CHK_PRM_RET(!tmp, goto wait,
                            "storage::updater::routine: Failed on get tkn.batch.last.balance for group \"%s\". Error: %s", group.data(), blns_reader.stringify(tmp).c_str())

                tmp = blns_reader.get("data", blns_reader.get_doc());
                if (tmp == nullptr) {
                    LOG_ERR("storage::updater::routine: Could not get 'data' from tkn.batch.last.balance");
                    goto wait;
                }
                if (!blns_reader.get_value(*tmp, "last_block", tmp_blocks.tkn_balance)) {
                    LOG_ERR("storage::updater::routine: Could not get 'last_block' from tkn.batch.last.balance");
                    goto wait;
                }

                blnses = blns_reader.get("balances", *tmp);
                if (blnses == nullptr || !blnses->IsArray()) {
                    LOG_ERR("storage::updater::routine: Could not get 'balances' from tkn.batch.last.balance or wrong type");
                    goto wait;
                }
                addresses.clear();
                for (auto& bl: blnses->GetArray()) {
                    addr.clear();
                    if (!blns_reader.get_value(bl, "address", addr)) {
                        LOG_ERR("storage::updater::routine: Could not get 'balances->address' from tkn.batch.last.balance");
                        goto wait;
                    }
                    hash_str.clear();
                    if (!storage::database::get_hash_tkn_balance(addr, hash_str)) {
                        if (!storage::database::get_last_status().IsNotFound()) {
                            LOG_ERR("storage::updater::routine: Failed on get hash for token balance. Address: %s", addr.c_str());
                            goto wait;
                        }
                    }
                    value = blns_reader.get("value", bl);
                    if (value == nullptr || !value->IsArray()) {
                        LOG_ERR("storage::updater::routine: Could not get 'balances->value' from tkn.batch.last.balance or wrong type");
                        goto wait;
                    }

                    temp_str.clear();
                    temp_str = blns_reader.stringify(value);

                    MD5((const unsigned char*)temp_str.c_str(), temp_str.size(), md5hash);
                    string_utils::bin2hex(md5hash, sizeof(md5hash), md5str);

                    if (hash_str.compare(md5str) != 0) {
                        storage::database::set_tkn_balance(addr, temp_str, md5str);
                        addresses.emplace_back(addr);
                        LOG_INF("storage::updater::routine: Token balance for \"%s\" was updated", addr.data());
                    }

//                    for (auto& v: value->GetArray()) {
//                        addr.clear();
//                        if (!blns_reader.get_value(v, "address", addr)) {
//                            LOG_ERR("storage::updater::routine: Could not get 'value->address' from tkn.batch.last.balance");
//                            continue;
//                        }
//                        std::transform(addr.begin(), addr.end(), addr.begin(), ::tolower);

//                        if (!blns_reader.get_value(v, "received", recvd)) {
//                            LOG_ERR("storage::updater::routine: Could not get 'value->received' from tkn.batch.last.balance");
//                            continue;
//                        }
//                        if (!blns_reader.get_value(v, "spent", spent)) {
//                            LOG_ERR("storage::updater::routine: Could not get 'value->spent' from tkn.batch.last.balance");
//                            continue;
//                        }
//                        db_blns.clear();
//                        if (!storage::database::get_hash_tkn_balance(addr, db_blns)) {
//                            if (!storage::database::get_last_status().IsNotFound()) {
//                                LOG_ERR("storage::updater::routine: Failed on get hash for token balance from db. Address: %s", addr.c_str());
//                                continue;
//                            }
//                        }
//                        if (!db_blns.empty()) {
//                            if (!db_reader.parse(db_blns.c_str())) {
//                                LOG_ERR("storage::updater::routine: Failed on parse db token balance. Parse error: %u", db_reader.get_parse_error().Code());
//                                continue;
//                            }
//                            if (!db_reader.get_value(db_reader.get_doc(), "received", db_recvd)) {
//                                LOG_ERR("storage::updater::routine: Could not found 'received' in db token balance");
//                                continue;
//                            }
//                            if (!db_reader.get_value(db_reader.get_doc(), "spent", db_spent)) {
//                                LOG_ERR("storage::updater::routine: Could not found 'spent' in db token balance");
//                                continue;
//                            }
//                            if (db_recvd.compare(recvd) == 0 && db_spent.compare(spent) == 0) {
//                                continue;
//                            }
//                        }

//                        storage::database::set_tkn_balance(addr, blns_reader.stringify(&v));
//                        addresses.emplace_back(addr);
//                        LOG_INF("storage::updater::routine: Token balance for \"%s\" was updated", addr.data());
//                    }
                }
                if (!addresses.empty()) {

                    response = perform<batch_history_tkn>(nullptr,
                        string_utils::str_concat("{\"id\":1, \"params\":{\"block\": ", std::to_string(blocks.tkn_history), ", \"group\":\"", group, "\"}}"));

                    if (response.message.empty()) {
                        LOG_ERR("storage::updater::routine: Failed on get token batch history for group \"%s\". Empty result.", group.data());
                        goto wait;
                    }
                    if (!his_reader.parse(response)) {
                        LOG_ERR("storage::updater::routine: Failed on parse token batch history. Parse error: %d", his_reader.get_parse_error().Code());
                        goto wait;
                    }
                    tmp = his_reader.get_error();
                    if (tmp != nullptr) {
                        LOG_ERR("storage::updater::routine: Failed on get token batch history for group \"%s\". Error: %s", group.data(), his_reader.stringify(tmp).c_str());
                        goto wait;
                    }
                    tmp = his_reader.get("data", his_reader.get_doc());
                    if (tmp == nullptr) {
                        LOG_ERR("storage::updater::routine: Could not get 'data' from token batch history");
                        goto wait;
                    }
                    if (!his_reader.get_value(*tmp, "last_block", tmp_blocks.tkn_history)) {
                        LOG_ERR("storage::updater::routine: Could not get 'last_block' from token batch history");
                        goto wait;
                    }

                    history = his_reader.get("history", *tmp);
                    if (history == nullptr) {
                        LOG_ERR("storage::updater::routine: Could not get 'history' from token batch history");
                        goto wait;
                    }
                    for (const auto& addr: addresses) {
                        for (auto his_it = history->GetArray().Begin(); his_it != history->GetArray().End(); ++his_it) {
                            addr_it = his_it->FindMember("address");
                            if (addr_it == his_it->MemberEnd()) {
                                LOG_ERR("storage::updater::routine: Can not find ID field 'history->address' in token batch history");
                                goto wait;
                            }

                            his_addr = addr_it->value.GetString();
                            std::transform(his_addr.begin(), his_addr.end(), his_addr.begin(), ::tolower);

                            if (his_addr.compare(addr) != 0) {
                                continue;
                            }
                            auto value = his_it->FindMember("value");
                            if (value == his_it->MemberEnd() && !value->value.IsArray()) {
                                LOG_ERR("storage::updater::routine: Can not find field 'history->value' in token batch history. Address: %s", addr.c_str());
                                break;
                            }
                            if (value->value.Empty()) {
                                break;
                            }
                            temp_str.clear();
                            //string_utils::str_append(temp_str, "{\"data\":", his_reader.stringify(&value->value), "}");
                            temp_str = his_reader.stringify(&value->value);

                            MD5((const unsigned char*)temp_str.c_str(), temp_str.size(), md5hash);
                            string_utils::bin2hex(md5hash, sizeof(md5hash), md5str);

                            if (storage::database::get_hash_tkn_history(addr, hash_str)) {
                                if (hash_str.compare(md5str) == 0) {
                                    break;
                                }
                            }
                            storage::database::set_tkn_history(addr, temp_str, md5str);
                            LOG_INF("storage::updater::routine: token history for \"%s\" was updated", addr.c_str());
                            break;
                        }
                    }
                }
wait:
                addresses::set_last_blocks(blocks, tmp_blocks);
                std::this_thread::sleep_until(tp);
            }

        } catch (std::exception& ex) {
            LOG_ERR("storage::updater::routine: Exception: %s", ex.what());
            return;
        } catch (invalid_param& ex) {
            LOG_ERR("storage::updater::routine: Exception: %s", ex.what().data());
            return;
        } catch (...) {
            LOG_ERR("storage::updater::routine: Unhandled exception");
            return;
        }
    }

}
