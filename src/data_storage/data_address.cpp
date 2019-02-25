#include "data_address.h"
#include "settings/settings.h"
#include "log/log.h"
#include <fstream>
//#include <boost/filesystem.hpp>
#include "common/filesystem_utils.h"
#include "common/string_utils.h"
#include <sys/stat.h>
#include <uuid/uuid.h>
#include <rapidjson/istreamwrapper.h>
#include "rapidjson/prettywriter.h"
#include <memory>

//namespace bf = boost::filesystem;

namespace storage
{
    const char* tracking_sufix = "eth-cli";

    std::mutex addresses::_locker;
    std::vector<std::string> addresses::_storage;
    std::string addresses::_group;
    addresses::last_blocks addresses::m_last_blocks;

    bool addresses::read_file(rapidjson::Document& doc)
    {
        if (settings::system::data_storage.empty()) {
            LOG_ERR("storage::address::read_file: Data storage path not defined");
            return false;
        }
        if (!fs_utils::dir::is_exists(settings::system::data_storage.c_str())) {
            if (!fs_utils::dir::create(settings::system::data_storage.c_str())) {
                LOG_ERR("storage::address::read_file: Could not create data storage path %s: %s",
                        settings::system::data_storage.c_str(), strerror(errno));
                return false;
            }
        }

        std::string path = string_utils::str_concat(settings::system::data_storage, "/addresses");
        if (!fs_utils::dir::is_exists(path.c_str())) {
            if (!fs_utils::dir::create(path.c_str())) {
                LOG_ERR("storage::address::read_file: Could not create adresses path %s: %s", path.c_str(), strerror(errno));
                return false;
            }
        }

        path.append("/tracking");
        std::fstream fs;
        fs.open(path.c_str());
        if (!fs.is_open()) {
            fs.open(path.c_str(), std::fstream::out);
            if (!fs.is_open()) {
                LOG_ERR("storage::address::read_file: Could not open tracking file %s: %s", path.c_str(), strerror(errno));
                return false;
            }
            fs << "{}";
            fs.flush();
            fs.close();
            fs.open(path.c_str());
            if (!fs.is_open()) {
                LOG_ERR("storage::address::read_file: Could not open tracking file %s: %s", path.c_str(), strerror(errno));
                return false;
            }
        }

        rapidjson::IStreamWrapper isw(fs);
        doc.ParseStream(isw);
        fs.close();

        if (doc.HasParseError()) {
            LOG_ERR("storage::address::read_file: Parse tracking file error: %d", doc.GetParseError());
            return false;
        }

        return true;
    }

    bool addresses::write_file(rapidjson::Document& doc)
    {
        if (settings::system::data_storage.empty()) {
            LOG_ERR("storage::address::write_file: Data storage path not defined");
            return false;
        }
        if (!fs_utils::dir::is_exists(settings::system::data_storage.c_str())) {
            if (!fs_utils::dir::create(settings::system::data_storage.c_str())) {
                LOG_ERR("storage::address::read_file: Could not create data storage path %s: %s", settings::system::data_storage.c_str(), strerror(errno));
                return false;
            }
        }
        std::string path = string_utils::str_concat(settings::system::data_storage, "/addresses");
        if (!fs_utils::dir::is_exists(path.c_str())) {
            if (!fs_utils::dir::create(path.c_str())) {
                LOG_ERR("storage::address::read_file: Could not create adresses path %s: %s", path.c_str(), strerror(errno));
                return false;
            }
        }

        path.append("/tracking");
        std::fstream fs;

        fs.open(path.c_str(), std::ios::out);
        if (!fs.is_open()) {
            LOG_ERR("storage::address::write_file: Could not open tracking file %s: %s", path.c_str(), strerror(errno));
            return false;
        }

        rapidjson::StringBuffer buf;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buf);
        doc.Accept(writer);
        fs << buf.GetString();
        fs.flush();
        fs.close();
        return true;
    }

    void addresses::init() {
        std::lock_guard<std::mutex> guard(_locker);
        _storage.clear();

        rapidjson::Document doc;
        if (!read_file(doc)) {
            return;
        }

        bool rewrite = false;

        auto tmp = doc.FindMember("group-id");
        if (tmp == doc.MemberEnd()) {
            doc.AddMember("group-id", rapidjson::Type::kStringType, doc.GetAllocator());
            tmp = doc.FindMember("group-id");
        }

        if (tmp->value.GetStringLength() == 0) {
            uuid_t uuid;
            uuid_generate_time(uuid);
            const size_t uuid_sz = sizeof(uuid_t)*2 + 6;
            std::unique_ptr<char[]> str(new char[uuid_sz]());
            memset(str.get(), 0, uuid_sz);
            uuid_unparse(uuid, str.get());

            _group.assign(str.get(), std::min(strlen(str.get()), uuid_sz));
            tmp->value.SetString(_group.c_str(), doc.GetAllocator());

            rewrite = true;
        } else {
            _group = tmp->value.GetString();
        }

        tmp = doc.FindMember("balance-lastblock");
        if (tmp == doc.MemberEnd()) {
            doc.AddMember("balance-lastblock", rapidjson::Type::kNumberType, doc.GetAllocator());
            tmp = doc.FindMember("balance-lastblock");
            tmp->value.SetUint64(0);
            rewrite = true;
        }
        m_last_blocks.balance = tmp->value.GetUint64();

        tmp = doc.FindMember("history-lastblock");
        if (tmp == doc.MemberEnd()) {
            doc.AddMember("history-lastblock", rapidjson::Type::kNumberType, doc.GetAllocator());
            tmp = doc.FindMember("history-lastblock");
            tmp->value.SetUint64(0);
            rewrite = true;
        }
        m_last_blocks.history = tmp->value.GetUint64();

        tmp = doc.FindMember("balance-tkn-lastblock");
        if (tmp == doc.MemberEnd()) {
            doc.AddMember("balance-tkn-lastblock", rapidjson::Type::kNumberType, doc.GetAllocator());
            tmp = doc.FindMember("balance-tkn-lastblock");
            tmp->value.SetUint64(0);
            rewrite = true;
        }
        m_last_blocks.tkn_balance = tmp->value.GetUint64();

        tmp = doc.FindMember("history-tkn-lastblock");
        if (tmp == doc.MemberEnd()) {
            doc.AddMember("history-tkn-lastblock", rapidjson::Type::kNumberType, doc.GetAllocator());
            tmp = doc.FindMember("history-tkn-lastblock");
            tmp->value.SetUint64(0);
            rewrite = true;
        }
        m_last_blocks.tkn_history = tmp->value.GetUint64();

        if (rewrite) {
            write_file(doc);
        }

        if (_group.empty()) {
            LOG_ERR("storage::address::init: Group can not be empty");
            return;
        }

        _group.append(":");
        _group.append(tracking_sufix);

        std::string str;
        auto addresses = doc.FindMember("addresses");
        if (addresses != doc.MemberEnd()) {
            for (const auto& addr: addresses->value.GetArray()) {
                if (addr.GetStringLength() == 0) {
                    continue;
                }
                str = addr.GetString();
                std::transform(str.begin(), str.end(), str.begin(), ::tolower);
                _storage.emplace_back(str);
            }
        }

        LOG_INF("storage::address::init: group=%s, addresses=%u, last.balance=%u, last.history=%u, last.tkn.balance=%u, last.tkn.history=%u",
                _group.c_str(), _storage.size(), m_last_blocks.balance, m_last_blocks.history,
                m_last_blocks.tkn_balance, m_last_blocks.tkn_history);
    }

    bool addresses::store(const std::string& address, bool reset) {
        std::lock_guard<std::mutex> guard(_locker);

        if (std::find(_storage.begin(), _storage.end(), address) != _storage.end()) {
            LOG_WRN("storage::address::store: Address %s is exists", address.c_str());
            return false;
        }

        rapidjson::Document doc;
        if (!read_file(doc)) {
            LOG_WRN("storage::address::store: Could not open tracking file");
            return false;
        }

        auto addresses = doc.FindMember("addresses");
        if (addresses == doc.MemberEnd()) {
            doc.AddMember("addresses", rapidjson::Type::kArrayType, doc.GetAllocator());
            addresses = doc.FindMember("addresses");
        }
        addresses->value.PushBack(rapidjson::Value().SetString(address.c_str(), doc.GetAllocator()), doc.GetAllocator());

        if (reset) {
            m_last_blocks = last_blocks();

            auto tmp = doc.FindMember("balance-lastblock");
            if (tmp == doc.MemberEnd()) {
                doc.AddMember("balance-lastblock", rapidjson::Type::kNumberType, doc.GetAllocator());
                tmp = doc.FindMember("balance-lastblock");
            }
            tmp->value.SetUint64(m_last_blocks.balance);
            tmp = doc.FindMember("history-lastblock");
            if (tmp == doc.MemberEnd()) {
                doc.AddMember("history-lastblock", rapidjson::Type::kNumberType, doc.GetAllocator());
                tmp = doc.FindMember("history-lastblock");
            }
            tmp->value.SetUint64(m_last_blocks.history);
            tmp = doc.FindMember("balance-tkn-lastblock");
            if (tmp == doc.MemberEnd()) {
                doc.AddMember("balance-tkn-lastblock", rapidjson::Type::kNumberType, doc.GetAllocator());
                tmp = doc.FindMember("balance-tkn-lastblock");
            }
            tmp->value.SetUint64(m_last_blocks.tkn_balance);
            tmp = doc.FindMember("history-tkn-lastblock");
            if (tmp == doc.MemberEnd()) {
                doc.AddMember("history-tkn-lastblock", rapidjson::Type::kNumberType, doc.GetAllocator());
                tmp = doc.FindMember("history-tkn-lastblock");
            }
            tmp->value.SetUint64(m_last_blocks.tkn_history);
            LOG_INF("storage::address::store: Blocks has been reseted");
        }

        if (!write_file(doc)) {
            LOG_WRN("storage::address::store: Could not rewrite tracking file");
            return false;
        }

        _storage.push_back(address);
        LOG_INF("storage::address::store: Address %s has been added", address.c_str());

        return true;
    }

    bool addresses::remove(const std::string& address) {
        std::lock_guard<std::mutex> guard(_locker);

        auto pos = std::find(_storage.begin(), _storage.end(), address);
        if (pos == _storage.end()) {
            LOG_WRN("storage::address::store: Address does not in storage");
            return false;
        }

        rapidjson::Document doc;
        if (!read_file(doc)) {
            LOG_WRN("storage::address::remove: Could not rewrite tracking file");
            return false;
        }

        auto addresses = doc.FindMember("addresses");
        if (addresses == doc.MemberEnd()) {
            LOG_WRN("storage::address::remove: 'addresses' field not found");
            return false;
        }

        auto it = std::find(addresses->value.GetArray().begin(), addresses->value.GetArray().end(), address.c_str());
        if (it == addresses->value.GetArray().end()) {
            LOG_WRN("storage::address::remove: address '%s'", address.c_str());
            return false;
        }
        addresses->value.GetArray().Erase(it);

        if (!write_file(doc)) {
            LOG_WRN("storage::address::remove: Could not rewrite tracking file");
            return false;
        }

        _storage.erase(pos);
        LOG_INF("storage::address::remove: Address %s has been removed", address.c_str());

        return true;
    }

    bool addresses::peek(size_t pos, std::string& result) {
        std::lock_guard<std::mutex> guard(_locker);
        if (pos >= _storage.size()) {
            return false;
        }
        result = _storage[pos];
        return true;
    }

    bool addresses::check(const std::string& address) {
        std::lock_guard<std::mutex> guard(_locker);
        return std::find(_storage.begin(), _storage.end(), address) != _storage.end();
    }

    addresses::storage_type addresses::snapshot() {
        std::lock_guard<std::mutex> guard(_locker);
        return _storage;
    }

    addresses::last_blocks addresses::get_last_blocks() {
        std::lock_guard<std::mutex> guard(_locker);
        return m_last_blocks;
    }

    void addresses::set_last_blocks(const addresses::last_blocks& prev, const addresses::last_blocks& vals) {
        std::lock_guard<std::mutex> guard(_locker);
        if (m_last_blocks == prev) {
            if (m_last_blocks == vals) {
                return;
            }

            m_last_blocks = vals;

            LOG_INF("storage::address::set_last_blocks: Blocks was updated. last.balance=%u, last.history=%u, last.tkn.balance=%u, last.tkn.history=%u",
                    m_last_blocks.balance, m_last_blocks.history, m_last_blocks.tkn_balance, m_last_blocks.tkn_history);

            rapidjson::Document doc;
            if (!read_file(doc)) {
                return;
            }
            auto tmp = doc.FindMember("balance-lastblock");
            if (tmp == doc.MemberEnd()) {
                doc.AddMember("balance-lastblock", rapidjson::Type::kNumberType, doc.GetAllocator());
                tmp = doc.FindMember("balance-lastblock");
            }
            tmp->value.SetUint64(m_last_blocks.balance);
            tmp = doc.FindMember("history-lastblock");
            if (tmp == doc.MemberEnd()) {
                doc.AddMember("history-lastblock", rapidjson::Type::kNumberType, doc.GetAllocator());
                tmp = doc.FindMember("history-lastblock");
            }
            tmp->value.SetUint64(m_last_blocks.history);
            tmp = doc.FindMember("balance-tkn-lastblock");
            if (tmp == doc.MemberEnd()) {
                doc.AddMember("balance-tkn-lastblock", rapidjson::Type::kNumberType, doc.GetAllocator());
                tmp = doc.FindMember("balance-tkn-lastblock");
            }
            tmp->value.SetUint64(m_last_blocks.tkn_balance);
            tmp = doc.FindMember("history-tkn-lastblock");
            if (tmp == doc.MemberEnd()) {
                doc.AddMember("history-tkn-lastblock", rapidjson::Type::kNumberType, doc.GetAllocator());
                tmp = doc.FindMember("history-tkn-lastblock");
            }
            tmp->value.SetUint64(m_last_blocks.tkn_history);
            write_file(doc);
        }
    }
}
