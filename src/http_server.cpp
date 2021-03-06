#include "http_server.h"
#include "http_session.h"
#include "settings/settings.h"
#include "log/log.h"
#include "task_handlers/task_handlers.h"
#include "json_rpc.h"
#include "data_storage/data_updater.h"
#include <iostream>
#include "connection_pool.h"
#include <boost/exception/all.hpp>
//#include <boost/bind.hpp>

std::unique_ptr<socket_pool> g_conn_pool;

http_server::http_server(unsigned short port /*= 9999*/, int thread_count /*= 4*/)
    : m_run(false)
    , m_thread_count(thread_count)
    , m_io_ctx(m_thread_count)
{
    m_ep.port(port);
}

http_server::~http_server()
{
}

void http_server::checkTimeout() {
}

void http_server::run()
{
    tcp::acceptor acceptor(m_io_ctx, m_ep, true);

    // Implements a custom socket option that determines whether or not an accept operation is permitted
    // to fail with boost::asio::error::connection_aborted. By default the option is false.
    // TODO check this
    //boost::asio::socket_base::enable_connection_aborted option(true);
    //acceptor.set_option(option);

    accept(acceptor);

    g_conn_pool = std::make_unique<socket_pool>();
    if (settings::system::conn_pool_enable) {
        g_conn_pool->capacity(settings::system::conn_pool_capacity);
        g_conn_pool->run_monitor();
    }

    m_run = true;
    std::vector<std::unique_ptr<std::thread> > threads;
    for (int i = 0; i < m_thread_count; ++i) {
        threads.emplace_back(new std::thread(worker_proc, this));
    }

    if (settings::service::local_data) {
        storage::updater::run();
    }

    LOG_INF("Service runing at %s:%d", m_ep.address().to_string().c_str(), m_ep.port());
    std::cout << "Service runing at " << m_ep.address().to_string() << ":" << m_ep.port() << std::endl;

    for (std::size_t i = 0; i < threads.size(); ++i) {
        threads[i]->join();
    }

    m_run = false;
    storage::updater::stop();

    g_conn_pool->stop_monitor();
    g_conn_pool.reset();

    LOG_INF("Service stoped")
}

void http_server::stop()
{
    m_run = false;
    m_io_ctx.stop();
}

void http_server::accept(tcp::acceptor& acceptor)
{
    acceptor.async_accept([&](boost::system::error_code ec, tcp::socket socket)
    {
        if (ec) {
            LOG_ERR("Accept failed: %s", ec.message().c_str())
        }
        else {
            boost::system::error_code er;
            const tcp::endpoint& ep = socket.remote_endpoint(er);
            if (er) {
                LOG_ERR("Accept error: %s", er.message().c_str());
                er.clear();
                socket.shutdown(tcp::socket::shutdown_both, er);
                socket.close(er);
            } else {
                if (check_access(ep)) {
                    std::make_shared<http_session>(std::move(socket))->run();
                } else {
                    LOG_ERR("Accept reject connection: %s:%d", ep.address().to_string().c_str(), ep.port());
                    socket.shutdown(tcp::socket::shutdown_both, er);
                    socket.close(er);
                }
            }
        }
        accept(acceptor);
    });
}

bool http_server::check_access(const tcp::endpoint& ep)
{

    if (settings::service::any_conns) {
        return true;
    }

    if (ep.address().is_loopback()) {
        return true;
    }

    if (std::find(settings::service::access.begin(),
              settings::service::access.end(),
                  ep.address().to_string()) != settings::service::access.end()) {
        return true;
     }

    return false;
}

void http_server::worker_proc(http_server* param)
{
    param->routine();
}

void http_server::routine()
{
    while (m_run) {
        try {
            boost::system::error_code ec;
            m_io_ctx.run(ec);
            if (ec) {
                LOG_ERR("%s IO context error (%d): %s", __PRETTY_FUNCTION__, ec.value(), ec.message().c_str());
            }
            LOG_INF("%s Break", __PRETTY_FUNCTION__);
            break;
        } catch (boost::exception& ex) {
            LOG_ERR("%s boost exception: %s", __PRETTY_FUNCTION__, boost::diagnostic_information(ex).c_str());
        } catch (std::exception& ex) {
            LOG_ERR("%s std exception: %s", __PRETTY_FUNCTION__, ex.what());
        } catch (...) {
            LOG_ERR("%s unhandled exception", __PRETTY_FUNCTION__);
        }
        LOG_INF("%s Continue", __PRETTY_FUNCTION__);
    }
}
