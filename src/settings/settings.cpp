#include "settings.h"

#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/asio/ip/tcp.hpp>
#include "../log/log.h"

#define BOOST_ERROR_CODE_HEADER_ONLY
#include <boost/program_options.hpp>

namespace   pt      = boost::property_tree;
namespace	asio    = boost::asio;
namespace	ip      = boost::asio::ip;
using		tcp     = boost::asio::ip::tcp;
namespace   bf      = boost::filesystem;

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

    // server
    std::string server::address = {"https://app.metahash.io/api/metachains/"};

    // system
    std::string system::wallet_stotage  = { boost::filesystem::current_path().append("/wallet").c_str() };
    std::string system::data_storage    = { boost::filesystem::current_path().append("/data").c_str() };
    bool system::isLightKey = true;
    bool system::debug_mode = false;
    unsigned int system::jrpc_conn_timeout = 60000;
    unsigned int system::jrpc_timeout = 500;

    void read()
    {
        pt::ptree tree;
        auto path = boost::filesystem::current_path();
        path.append("/settings.json");
        pt::read_json(path.c_str(), tree);

        service::port           = tree.get<unsigned short>("service.port", 9999);
        service::threads        = tree.get<int>("service.threads", 4);
        service::token          = tree.get<std::string>("service.token", "");
        service::coin_key       = tree.get<int>("service.coin-key", -1);
        service::gas_price_min  = tree.get<std::string>("service.gas-price-min", "0");
        service::gas_price_max  = tree.get<std::string>("service.gas-price-max", "0");
        service::local_data     = tree.get<bool>("service.localdata", false);

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

        system::wallet_stotage  = tree.get<std::string>("system.wallets-storage", boost::filesystem::current_path().append("/wallet").c_str());
        system::data_storage    = tree.get<std::string>("system.data-storage", boost::filesystem::current_path().append("/data").c_str());
        system::isLightKey      = tree.get<bool>("system.is-light-key", true);
        system::jrpc_conn_timeout = tree.get<unsigned int>("system.jrpc-conn-timeout", 60000);
        system::jrpc_timeout    = tree.get<unsigned int>("system.jrpc-timeout", 500);
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
