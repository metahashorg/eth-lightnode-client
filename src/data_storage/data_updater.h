#ifndef DATA_UPDATER_H_
#define DATA_UPDATER_H_

#define RAPIDJSON_HAS_STDSTRING 1

#include <thread>

namespace storage
{
    class updater
    {
    public:
        updater() = delete;
        updater(const updater&) = delete;
        updater(const updater&&) = delete;
        updater& operator = (const updater&) = delete;
        updater& operator = (const updater&&) = delete;
        ~updater() {}

        static void run();
        static void stop();

    protected:
        static void routine();

        static bool prepare();
        static void update_data();

    protected:
        static std::unique_ptr<std::thread> m_thr;
        static bool m_run;
    };
}



#endif // DATA_UPDATER_H_

