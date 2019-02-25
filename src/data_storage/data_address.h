#ifndef DATA_ADDRESS_H_
#define DATA_ADDRESS_H_

#include <string.h>
#include <string>
#include <vector>
#include <mutex>
//include <memory>
//#include <fstream>
#include "rapidjson/document.h"

namespace storage
{
    extern const char* tracking_sufix;

    class addresses
    {
    public:
        typedef std::vector<std::string> storage_type;

        struct last_blocks {
            uint64_t balance = {0};
            uint64_t tkn_balance = {0};
            uint64_t history = {0};
            uint64_t tkn_history = {0};

            bool operator ==(const last_blocks& rhs) const {
                return memcmp(this, &rhs, sizeof(last_blocks)) == 0;
            }
        };

        addresses() = delete;
        addresses(const addresses&) = delete;
        addresses(const addresses&&) = delete;
        addresses& operator = (const addresses&) = delete;
        addresses& operator = (const addresses&&) = delete;

        static void init();
        static bool store(const std::string& address, bool reset = true);
        static bool peek(size_t pos, std::string& result);
        static bool remove(const std::string& address);
        static bool check(const std::string& address);

        static storage_type snapshot();

        static const std::string_view group() {
            return _group;
        }
        static void group(const std::string& value) {
            _group = value;;
        }

        static last_blocks get_last_blocks();
        static void set_last_blocks(const addresses::last_blocks& prev, const addresses::last_blocks& vals);

    protected:
        static bool write_file(rapidjson::Document& doc);
        static bool read_file(rapidjson::Document& doc);

    protected:
        static std::mutex _locker;
        static storage_type _storage;
        static std::string _group;
        static last_blocks m_last_blocks;
    };
}

#endif // DATA_ADDRESS_H_
