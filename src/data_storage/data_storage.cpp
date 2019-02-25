#include "data_storage.h"
#include "settings/settings.h"
#include "data_log.h"
#include "common/filesystem_utils.h"
#include "leveldb/db.h"
#include "common/string_utils.h"

//namespace bf = boost::filesystem;

namespace storage
{
    std::unique_ptr<leveldb::DB> database::_db;
    std::mutex database::_locker;
    leveldb::Status database::_status;

    bool database::open() {
        std::lock_guard<std::mutex> guard(_locker);
        if (_db != nullptr) {
            return true;
        }
        leveldb::Options options;
        options.create_if_missing = true;

        if (!fs_utils::dir::is_exists(settings::system::data_storage.c_str())) {
            if (!fs_utils::dir::create(settings::system::data_storage.c_str())) {
                LOG_ERR("storage::database::open: Could not create data storage path");
                return false;
            }
        }

        std::string path = string_utils::str_concat(settings::system::data_storage, "/db");
        if (!fs_utils::dir::is_exists(path.c_str())) {
            if (!fs_utils::dir::create(path.c_str())) {
                LOG_ERR("storage::database::open: Could not create db path");
                return false;
            }
        }
        leveldb::DB* db = nullptr;
        _status = leveldb::DB::Open(options, path.c_str(), &db);
        if (_status.ok()) {
            _db.reset(db);
            return true;
        }
        LOG_ERR("storage::database: Could not open database: %s", _status.ToString().c_str());
        return false;
    }

    bool database::get_hash_balance(const std::string_view& address, std::string& result) {
        std::lock_guard<std::mutex> guard(_locker);
        _status = _db->Get(leveldb::ReadOptions(), string_utils::str_concat(address, "$hash_blns"), &result);
        if (!_status.ok()) {
            LOG_WRN("storage::database: Could not get hash for balance address \"%s\". Error: %s", address.data(), _status.ToString().c_str());
            return false;
        }
        return true;
    }

    bool database::get_hash_history(const std::string_view& address, std::string& result) {
        std::lock_guard<std::mutex> guard(_locker);
        _status = _db->Get(leveldb::ReadOptions(), string_utils::str_concat(address, "$hash_hist"), &result);
        if (!_status.ok()) {
            LOG_WRN("storage::database: Could not get hash for history address \"%s\". Error: %s", address.data(), _status.ToString().c_str());
            return false;
        }
        return true;
    }

    bool database::get_hash_tkn_balance(const std::string_view& address, std::string& result) {
        std::lock_guard<std::mutex> guard(_locker);
        _status = _db->Get(leveldb::ReadOptions(), string_utils::str_concat(address, "$hash_tkn_blns"), &result);
        if (!_status.ok()) {
            LOG_WRN("storage::database: Could not get hash for balance address \"%s\". Error: %s", address.data(), _status.ToString().c_str());
            return false;
        }
        return true;
    }

    bool database::get_hash_tkn_history(const std::string_view& address, std::string& result) {
        std::lock_guard<std::mutex> guard(_locker);
        _status = _db->Get(leveldb::ReadOptions(), string_utils::str_concat(address, "$hash_tkn_hist"), &result);
        if (!_status.ok()) {
            LOG_WRN("storage::database: Could not get hash for history address \"%s\". Error: %s", address.data(), _status.ToString().c_str());
            return false;
        }
        return true;
    }

    bool database::get_balance(const std::string_view& address, std::string& result) {
        std::lock_guard<std::mutex> guard(_locker);
        _status = _db->Get(leveldb::ReadOptions(), string_utils::str_concat(address, "$blns"), &result);
        if (!_status.ok()) {
            LOG_WRN("storage::database: Could not get balance for address \"%s\". Error: %s", address.data(), _status.ToString().c_str());
            return false;
        }
        return true;
    }

    void database::set_balance(const std::string_view& address, const std::string& value, const char* hash) {
        std::lock_guard<std::mutex> guard(_locker);
        _status = _db->Put(leveldb::WriteOptions(), string_utils::str_concat(address, "$blns"), value);
        if (!_status.ok()) {
            LOG_WRN("storage::database: Could not save balance for address \"%s\". Error: %s", address.data(), "\". Error: ", _status.ToString().c_str());
            return;
        }
        _status = _db->Put(leveldb::WriteOptions(), string_utils::str_concat(address, "$hash_blns"), hash);
        if (!_status.ok()) {
            LOG_WRN("storage::database: Could not save hash for balance address \"%s\". Error: %s", address.data(), "\". Error: ", _status.ToString().c_str());
            return;
        }
    }

    bool database::get_history(const std::string_view& address, std::string& result) {
        std::lock_guard<std::mutex> guard(_locker);
        _status = _db->Get(leveldb::ReadOptions(), string_utils::str_concat(address, "$hist"), &result);
        if (!_status.ok()) {
            LOG_WRN("storage::database: Could not get history for address \"%s\". Error: %s", address.data(), _status.ToString().c_str());
            return false;
        }
        return true;
    }

    void database::set_history(const std::string_view& address, const std::string& value, const char* hash) {
        std::lock_guard<std::mutex> guard(_locker);
        _status = _db->Put(leveldb::WriteOptions(), string_utils::str_concat(address, "$hist"), value);
        if (!_status.ok()) {
            LOG_WRN("storage::database: Could not set history for address \"%s\". Error: %s", address.data(), _status.ToString().c_str());
            return;
        }
        _status = _db->Put(leveldb::WriteOptions(), string_utils::str_concat(address, "$hash_hist"), hash);
        if (!_status.ok()) {
            LOG_WRN("storage::database: Could not save hash for history address \"%s\". Error: %s", address.data(), "\". Error: ", _status.ToString().c_str());
            return;
        }
    }

    bool database::get_tkn_history(const std::string_view& address, std::string& result) {
        std::lock_guard<std::mutex> guard(_locker);
        _status = _db->Put(leveldb::WriteOptions(), string_utils::str_concat(address, "$tkn_hist"), result);
        if (!_status.ok()) {
            LOG_WRN("storage::database: Could not get token history for address \"%s\". Error: %s", address.data(), _status.ToString().c_str());
        }
        return true;
    }

    void database::set_tkn_history(const std::string_view& address, const std::string& value, const char* hash) {
        std::lock_guard<std::mutex> guard(_locker);
        _status = _db->Put(leveldb::WriteOptions(), string_utils::str_concat(address, "$tkn_hist"), value);
        if (!_status.ok()) {
            LOG_WRN("storage::database: Could not set token history for address \"%s\". Error: %s", address.data(), _status.ToString().c_str());
            return;
        }
        _status = _db->Put(leveldb::WriteOptions(), string_utils::str_concat(address, "$hash_tkn_hist"), hash);
        if (!_status.ok()) {
            LOG_WRN("storage::database: Could not save hash for token history address \"%s\". Error: %s", address.data(), "\". Error: ", _status.ToString().c_str());
            return;
        }
    }

    bool database::get_tkn_balance(const std::string_view& address, std::string& result) {
        std::lock_guard<std::mutex> guard(_locker);
        _status = _db->Put(leveldb::WriteOptions(), string_utils::str_concat(address, "$tkn_blns"), result);
        if (!_status.ok()) {
            LOG_WRN("storage::database: Could not get token balance for address \"%s\". Error: %s", address.data(), _status.ToString().c_str());
        }
        return true;
    }
    void database::set_tkn_balance(const std::string_view& address, const std::string& value, const char* hash) {
        std::lock_guard<std::mutex> guard(_locker);
        _status = _db->Put(leveldb::WriteOptions(), string_utils::str_concat(address, "$tkn_blns"), value);
        if (!_status.ok()) {
            LOG_WRN("storage::database: Could not set token balance for address \"%s\". Error: %s", address.data(), _status.ToString().c_str());
            return;
        }
        _status = _db->Put(leveldb::WriteOptions(), string_utils::str_concat(address, "$hash_tkn_blns"), hash);
        if (!_status.ok()) {
            LOG_WRN("storage::database: Could not save hash for token balance address \"%s\". Error: %s", address.data(), "\". Error: ", _status.ToString().c_str());
            return;
        }
    }
}
