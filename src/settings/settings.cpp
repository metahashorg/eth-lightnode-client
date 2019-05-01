#include "settings.h"

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/asio/ip/tcp.hpp>
#include "../log/log.h"

#define BOOST_ERROR_CODE_HEADER_ONLY
#include <boost/program_options.hpp>

namespace   pt      = boost::property_tree;
namespace   asio    = boost::asio;
namespace   ip      = boost::asio::ip;
using       tcp     = boost::asio::ip::tcp;

namespace settings
{
    // service
    unsigned short service::port = {9999};
    int service::threads = {4};
    bool service::any_conns = {false};
    bool service::local_data = {false};
    std::string service::token;
    int service::coin_key = {-1};
    mpz_class service::gas_price_min;
    mpz_class service::gas_price_max;
    std::vector<std::string> service::access;
    bool service::keep_alive = {false};

    // server
    std::string server::address = {"https://app.metahash.io/api/metachains/"};

    // system
    std::string system::wallet_stotage  = { "./wallet" };
    std::string system::data_storage    = { "./data" };
    bool system::isLightKey = true;
    bool system::debug_mode = false;
    unsigned int system::jrpc_conn_timeout = 1000;
    unsigned int system::jrpc_timeout = 60000;
    bool system::conn_pool_enable = true;
    unsigned int system::conn_pool_ttl = 60;
    unsigned int system::conn_pool_capacity = 100;

    void read()
    {
        pt::ptree tree;
        pt::read_json("./settings.json", tree);

        service::port           = tree.get<unsigned short>("service.port", 9999);
        service::threads        = tree.get<int>("service.threads", 4);
        service::token          = tree.get<std::string>("service.token", "");
        service::coin_key       = tree.get<int>("service.coin-key", -1);
        service::gas_price_min  = tree.get<std::string>("service.gas-price-min", "0");
        service::gas_price_max  = tree.get<std::string>("service.gas-price-max", "0");
        service::local_data     = tree.get<bool>("service.localdata", false);
        service::keep_alive     = tree.get<bool>("service.keep-alive", false);

        asio::io_context ctx;
        tcp::resolver resolver(ctx);
        boost::property_tree::ptree access;
        access = tree.get_child("service.access", access);
        for (auto &v : access)
        {
            boost::system::error_code er;
            auto eps = resolver.resolve({v.second.data(), ""}, er);
            if (er)
            {
                LOG_WRN("Couldn't resolve %s : %s", v.second.data().c_str(), er.message().c_str());
                continue;
            }
            for (auto &e : eps)
                service::access.push_back(e.endpoint().address().to_string());
        }

        server::address = tree.get<std::string>("server", "https://app.metahash.io/api/metachains/");

        system::wallet_stotage  = tree.get<std::string>("system.wallets-storage", "./wallet");
        system::data_storage    = tree.get<std::string>("system.data-storage", "./data");
        system::isLightKey      = tree.get<bool>("system.is-light-key", true);
        system::jrpc_conn_timeout = tree.get<unsigned int>("system.jrpc-conn-timeout", 1000);
        system::jrpc_timeout    = tree.get<unsigned int>("system.jrpc-timeout", 60000);

        system::conn_pool_enable = tree.get<bool>("system.conn_pool_enable", false);
        system::conn_pool_ttl = tree.get<unsigned int>("system.conn_pool_ttl", 60);
        system::conn_pool_capacity = tree.get<unsigned int>("system.conn_pool_capacity", 100);
    }

    void read(boost::program_options::variables_map& vm)
    {
        if (vm.count("any"))
            settings::service::any_conns = true;

        if (vm.count("threads"))
            settings::service::threads = std::max(vm["threads"].as<int>(), 1);

        if (vm.count("port"))
            settings::service::port = vm["port"].as<unsigned short>();

        if (vm.count("addr"))
            settings::server::address = vm["addr"].as<std::string>();

        if (vm.count("storage"))
            settings::system::wallet_stotage = vm["storage"].as<std::string>();
	
        if (vm.count("token"))
            settings::service::token = vm["token"].as<std::string>();

        if (vm.count("coin-key"))
            settings::service::coin_key = vm["coin-key"].as<int>();

        if (vm.count("debug"))
            settings::system::debug_mode = true;
    }
}
