#include <iostream>

#include "http_server.h"
#include "settings/settings.h"
#include "log/log.h"
#include "P7_Client.h"
#include <signal.h>
#include "common/signal_handler.h"
#include <execinfo.h>
#include "cmake_modules/GitSHA1.h"

#define BOOST_ERROR_CODE_HEADER_ONLY
#include <boost/program_options.hpp>

namespace po = boost::program_options;

static std::unique_ptr<http_server> server;

void signal_catcher(int sig);

int main(int argc, char* argv[])
{
    try {
        logg::init();

        std::cout << "Revision: " << g_GIT_SHA1 << std::endl;
        std::cout << "Build Date: " << g_GIT_DATE << std::endl;

        LOG_INF("Revision: ", g_GIT_SHA1);
        LOG_INF("Build Date: ", g_GIT_DATE);

        common::set_signal_handler(signal_catcher);

        settings::read();

        settings::system::debug_mode = true;

        po::options_description desc("Allowed options");
        desc.add_options()
            ("help",                                            "produce this help message")
            ("threads",         po::value<int>(),               "number of threads")
            ("port",            po::value<unsigned short>(),    "service port, default is 9999")
            ("server",          po::value<std::string>(),       "server address")
            ("storage",         po::value<std::string>(),       "path to storage of wallets")
            ("any",                                             "accept any connections")
            ("token",           po::value<std::string>(),       "sevice token")
            ("coin-key",        po::value<int>(),               "coin key identificator")
            ("debug",                                           "debug mode");

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help"))
        {
            std::cout << desc << std::endl;
            return EXIT_SUCCESS;
        }

        settings::read(vm);

        server = std::make_unique<http_server>(settings::service::port, settings::service::threads);
        server->run();

        logg::release();
        
        return EXIT_SUCCESS;
    }
    catch (const std::exception& e)
    {
        LOG_ERR(e.what());
        P7_Exceptional_Flush();
        return EXIT_FAILURE;
    }
    catch (...)
    {
        LOG_ERR("Unhandled exception");
        P7_Exceptional_Flush();
        return EXIT_FAILURE;
    }
}

#include <dlfcn.h>
void signal_catcher(int sig)
{
    std::string out;
    out.reserve(512);
    out.append("Caught signal \"");
    out.append(std::to_string(sig));
    out.append("\" : ");
    out.append(strsignal(sig));

    void* addrlist[40];
    int size = backtrace(addrlist, sizeof(addrlist)/sizeof(void*));

    for (int i=0; i < size; ++i){
        LOG_INF("addr: %u", addrlist[i]);
        Dl_info inf;
        dladdr(addrlist[i], &inf);
        LOG_INF("info %u, %s, %u, %s", inf.dli_fbase, inf.dli_fname, inf.dli_saddr, inf.dli_sname);
    }

    if (size != 0) {
        out.append("\nStack trace:\n");
        char** symbollist = backtrace_symbols(addrlist, size);
        for (int i = 0; i < size; ++i) {
            out.append("  ");
            out.append(symbollist[i]);
            out.append("\n");
        }
        free(symbollist);
    }
    LOG_ERR(out.c_str());
    P7_Exceptional_Flush();
    server->stop();
}
