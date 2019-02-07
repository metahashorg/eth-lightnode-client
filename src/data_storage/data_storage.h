#ifndef DATA_STORAGE_H_
#define DATA_STORAGE_H_

#include <string>
#include <memory>
#include <mutex>
#include "leveldb/db.h"

namespace storage
{
    class database
    {
    public:
        database() = delete;
        database(const database&) = delete;
        database(const database&&) = delete;
        database& operator = (const database&) = delete;
        database& operator = (const database&&) = delete;

        ~database(){}

        static bool open();

        static bool get_hash_balance(const std::string_view& address, std::string& result);
        static bool get_hash_history(const std::string_view& address, std::string& result);
        static bool get_hash_tkn_balance(const std::string_view& address, std::string& result);
        static bool get_hash_tkn_history(const std::string_view& address, std::string& result);

        static bool get_balance(const std::string_view& address, std::string& result);
        static void set_balance(const std::string_view& address, const std::string& value, const char* hash);

        static bool get_history(const std::string_view& address, std::string& result);
        static void set_history(const std::string_view& address, const std::string& value, const char* hash);

        static bool get_tkn_history(const std::string_view& address, std::string& result);
        static void set_tkn_history(const std::string_view& address, const std::string& value, const char* hash);

        static bool get_tkn_balance(const std::string_view& address, std::string& result);
        static void set_tkn_balance(const std::string_view& address, const std::string& value, const char* hash);

        inline static leveldb::Status get_last_status() {
            std::lock_guard<std::mutex> guard(_locker);
            return _status;
        }

    protected:
        static std::unique_ptr<leveldb::DB> _db;
        static std::mutex                   _locker;
        static leveldb::Status              _status;
    };
}

#endif // DATA_STORAGE_H_
