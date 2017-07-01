
#include "client_base.hpp"
#include "server_base.hpp"
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
#define MAX_LEN 4000
int main(void)
{
    LogManager::getLogger(logging_cb)->setLevel(Logger::WARN); //ALL); //WARN); //ALL);

    /************this is DEALER <->ROUTER MODE ************/
    {
        logger->error(ZMQ_LOG, " ************   this is DEALER<->ROUTER MODE************\n");

        /************server related ************/
        // init server
        st1.setIPPort("127.0.0.1:5571");
        st1.set_monitor_cb(server_monitor_func);
        // for server, you need to set callback function first
        st1.set_cb(server_cb_001);
        st1.run();
        /************client related ************/
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

        while (1)
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
    return 0;
}
