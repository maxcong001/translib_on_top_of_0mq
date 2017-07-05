
#include "client_base.hpp"
#include "server_base.hpp"
#include "broker.hpp"
#include "worker.hpp"
#include "util.hpp"
#include <string>
#include <thread> // std::this_thread::sleep_for
#include <chrono> // std::chrono::seconds
#include <mutex>
#include <future>
#include <atomic>
#include <stdlib.h>
#include <test_util.hpp>
// message len that we will send
#define MAX_LEN 20

void dealer_router_example();
void dealer_router_router_dealer_example();
int main(void)
{
    LogManager::getLogger(logging_cb)->setLevel(Logger::WARN); //ALL);

    dealer_router_example();

    dealer_router_router_dealer_example();

    return 0;
}
void dealer_router_router_dealer_example()
{
    /************this is DEALER <->(ROUTER<->ROUTER)<->DEALER MODE ************/
    {
        logger->error(ZMQ_LOG, " ************   this is DEALER <->(ROUTER<->ROUTER)<->DEALER MODE************\n");

        /************client related ************/
        logger->debug(ZMQ_LOG, "starting client now\n");
        // init client
        //prepare data
        char buffer[MAX_LEN];
        for (int i = 0; i < MAX_LEN; i++)
        {
            buffer[i] = 'a';
        }
        //test if binary safe
        //buffer[10] = 0;
        std::string test_str(buffer, MAX_LEN);
        void *user_data = (void *)28;
        // start some clients
        std::vector<client_base *> client_vector;
        for (int num = 0; num < 20; num++)
        {
            client_vector.emplace_back(new client_base());
        }

        for (auto tmp_client : client_vector)
        {
            tmp_client->set_monitor_cb(client_monitor_func);
            tmp_client->setIPPort("127.0.0.1:5561");
            tmp_client->run();
        }

        /************broker related ************/
        logger->debug(ZMQ_LOG, "starting broker now\n");
        broker_base bk1;
        bk1.set_backtend_protocol("ipc://");
        bk1.set_backtend_IPPort("abcdefg");
        auto broker_fun = std::bind(&broker_base::run, &bk1);
        std::thread broker_t(broker_fun);
        //broker_t.detach();

        /************worker related ************/
        logger->debug(ZMQ_LOG, "starting worker now\n");
        wk1.set_monitor_cb(server_monitor_func);
        // for server, you need to set callback function first
        wk1.set_cb(worker_cb_001);
        wk1.set_protocol("ipc://");
        wk1.setIPPort("abcdefg");
        wk1.run();

        /************send message ************/
        for (int time = 0; time < 1; time++)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            for (int i = 0; i < 10; i++)
            {
                for (auto tmp_client : client_vector)
                {
                    tmp_client->send(user_data, client_cb_001, test_str.c_str(), size_t(test_str.size()));
                }
            }
        }
        /************clean up ************/
    }
    broker_t.join();
}

void dealer_router_example()
{
    /************this is DEALER <->ROUTER MODE ************/
    {
        logger->error(ZMQ_LOG, " ************   this is DEALER<->ROUTER MODE************\n");

        /************server related ************/
        logger->debug(ZMQ_LOG, "starting server now\n");
        // init server
        st1.setIPPort("127.0.0.1:5571");
        st1.set_monitor_cb(server_monitor_func);
        // for server, you need to set callback function first
        st1.set_cb(server_cb_001);
        st1.run();
        /************client related ************/
        logger->debug(ZMQ_LOG, "starting client now\n");
        // init client
        //prepare data
        char buffer[MAX_LEN];
        for (int i = 0; i < MAX_LEN; i++)
        {
            buffer[i] = 'a';
        }

        std::string test_str(buffer, MAX_LEN);
        void *user_data = (void *)28;
        // start some clients
        std::vector<client_base *> client_vector;
        for (int num = 0; num < 20; num++)
        {
            client_vector.emplace_back(new client_base());
        }

        for (auto tmp_client : client_vector)
        {
            tmp_client->set_monitor_cb(client_monitor_func);
            tmp_client->setIPPort("127.0.0.1:5571");
            tmp_client->run();
        }

        for (int time = 0; time < 1000; time++)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            for (int i = 0; i < 40; i++)
            {
                for (auto tmp_client : client_vector)
                {
                    tmp_client->send(user_data, client_cb_001, test_str.c_str(), size_t(test_str.size()));
                }
            }
        }
    }
}
