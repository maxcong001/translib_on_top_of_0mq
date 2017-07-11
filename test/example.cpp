
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
#define MAX_LEN 4000
server_base st1;
worker_base wk1;
worker_base wk2;
worker_base wk3;

void server_cb_001(const char *data, size_t len, void *ID)
{
	/*
    std::cout
        << std::dec << "receive message form client : "
        << " total message: " << message_count++ << std::endl;
        */

	//std::this_thread::sleep_for(std::chrono::milliseconds(10));
	//std::cout << std::dec << " \[SERVER\] total message: " << message_count++ << " ID is " << (long)ID << std::endl;
	st1.send(data, len, ID);
}
void worker_cb_001(const char *data, size_t len, void *ID)
{
	//std::cout << std::dec << "\[WORKER 1\]receive message form client : " << (std::string(data, len)) << " total message: " << message_count++ << std::endl;
	//std::cout << std::dec << "\[WORKER 1\] total message: " << message_count++ << std::endl;
	wk1.send(data, len, ID);
}
void worker_cb_002(const char *data, size_t len, void *ID)
{
	//std::cout << std::dec << "\[WORKER 2\]receive message form client : " << (std::string(data, len)) << " total message: " << message_count++ << std::endl;
	//std::cout << std::dec << "\[WORKER 2\] total message: " << message_count++ << std::endl;
	wk2.send(data, len, ID);
}
void worker_cb_003(const char *data, size_t len, void *ID)
{
	//std::cout << std::dec << "\[WORKER 3\]receive message form client : " << (std::string(data, len)) << " total message: " << message_count++ << std::endl;
	//std::cout << std::dec << "\[WORKER 3\] total message: " << message_count++ << std::endl;
	wk3.send(data, len, ID);
}
void dealer_router_example();
void dealer_router_router_dealer_example();
int main(void)
{
	LogManager::getLogger(logging_cb)->setLevel(Logger::WARN); //ALL);

	//dealer_router_router_dealer_example();

	dealer_router_example();
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	logger->error(ZMQ_LOG, " ************   exit example ************\n");
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

		// start some clients
		std::vector<client_base *> client_vector;
		// should not be larger than 15
		for (int num = 0; num < 15; num++)
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
		broker_t.detach();

		/************worker related ************/
		logger->debug(ZMQ_LOG, "starting worker now\n");
		wk1.set_monitor_cb(server_monitor_func);
		// for server, you need to set callback function first
		wk1.set_cb(worker_cb_001);
		wk1.set_protocol("ipc://");
		wk1.setIPPort("abcdefg");
		wk1.run();

		wk2.set_monitor_cb(server_monitor_func);
		// for server, you need to set callback function first
		wk2.set_cb(worker_cb_002);
		wk2.set_protocol("ipc://");
		wk2.setIPPort("abcdefg");
		wk2.run();

		wk3.set_monitor_cb(server_monitor_func);
		// for server, you need to set callback function first
		wk3.set_cb(worker_cb_003);
		wk3.set_protocol("ipc://");
		wk3.setIPPort("abcdefg");
		wk3.run();
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));

		/************send message ************/
		for (int time = 0; time < 20; time++)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			for (int i = 0; i < 10; i++)
			{
				for (auto tmp_client : client_vector)
				{
					tmp_client->send(user_data, client_cb_001, test_str.c_str(), size_t(test_str.size()));
					user_data = ((char *)user_data) + 1;
				}
			}
		}
		logger->error(ZMQ_LOG, " ************   exit DEALER <->(ROUTER<->ROUTER)<->DEALER MODE************\n");
		std::this_thread::sleep_for(std::chrono::milliseconds(20000));
		/************clean up ************/
		for (auto tmp_client : client_vector)
		{
			if (tmp_client)
			{
				delete tmp_client;
			}
		}
		//bk1.stop();
	}
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
		void *user_data = (void *)0;
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

		for (int time = 0; time < 10; time++)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			for (int i = 0; i < 10; i++)
			{
				for (auto tmp_client : client_vector)
				{
					tmp_client->send(user_data, client_cb_001, test_str.c_str(), size_t(test_str.size()));
					user_data = ((char *)user_data) + 1;
				}
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(10000));
		/************clean up ************/
		for (auto tmp_client : client_vector)
		{
			if (tmp_client)
			{
				delete tmp_client;
			}
		}
	}
}
