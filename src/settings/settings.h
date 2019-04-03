#ifndef SETTINGS_H_
#define SETTINGS_H_

#include <string>
#include <vector>
#include <gmpxx.h>

namespace boost {
namespace program_options {
class variables_map;
}
}

namespace settings
{
    struct service
    {
        static int threads;
        static unsigned short port;
        static bool any_conns;
        static bool local_data;
        static int coin_key;
        static mpz_class gas_price_min;
        static mpz_class gas_price_max;
        static std::string token;
        static std::vector<std::string> access;
        static bool keep_alive;
    };

    struct server
    {
        static std::string address;
    };

    struct system
    {
        static std::string wallet_stotage;
        static std::string data_storage;
        static bool isLightKey;
        static bool debug_mode;
        static unsigned int jrpc_conn_timeout;
        static unsigned int jrpc_timeout;
    };

    void read();
    void read(boost::program_options::variables_map& vm);
}

#endif // SETTINGS_H_
