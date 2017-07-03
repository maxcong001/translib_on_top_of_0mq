
#include "client_base.hpp"
#include "server_base.hpp"
#include "util.hpp"
#include <string>
#include <thread> // std::this_thread::sleep_for
#include <chrono> // std::chrono::seconds
#include <mutex>
#include <future>

// this is the callback function of server
// typedef std::function<void(const char *, size_t, void *)> SERVER_CB_FUNC;
// this is the callback function of client
// typedef void USR_CB_FUNC(char *msg, size_t len, void *usr_data);
//server_base st1;
std::mutex mtx;
int count;
//std::lock_guard<std::mutex> lock(mtx);
void client_cb_001(const char *msg, size_t len, void *usr_data)
{
	std::cout << "receive message form server : \"" << (std::string(msg, len)) << "\" with user data : " << usr_data << " message count : " << count++ << std::endl;
}

/*
void server_cb_001(const char *data, size_t len, void *ID)
{
	std::cout << "receive message form client : " << (std::string(data, len)) << std::endl;
	st1.send(data, len, ID);
}*/
void client_monitor_func(int event, int value, std::string &address)
{
	std::cout << "receive event form client monitor task, the event is " << event << ". Value is : " << value << ". string is : " << address << std::endl;
}
void server_monitor_func(int event, int value, std::string &address)
{
	std::cout << "receive event form server monitor task, the event is " << event << ". Value is : " << value << ". string is : " << address << std::endl;
}
int main(void)
{
#if 0
	client_base ct1;
	ct1.set_monitor_cb(client_monitor_func);
	ct1.setIPPort("127.0.0.1:5561");

	//ct1.setIPPortSource("127.0.0.1:5591");

	//	client_base ct2;
	//	ct2.set_monitor_cb(client_monitor_func);
	//	ct2.setIPPort("127.0.0.1:5570");
	//ct1.setIPPortSource("127.0.0.1:5590");

	//st1.setIPPort("127.0.0.1:5570");
	//st1.set_monitor_cb(server_monitor_func);
	// for server, you need to set callback function first
	//st1.set_cb(server_cb_001);

	ct1.run();
	//ct2.run();

	//st1.run();
#endif
	std::string test_str = "this is for test!";
	void *user_data = (void *)28;
	std::cout << "send message : \"" << test_str << "\"  with usr data : " << user_data << std::endl;
	/*
	//for (int i = 0; i < 10; i++)
	while (1)
	{
		//getchar();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		for (int i = 0; i < 4; i++)
		{

			ct1.send(user_data, client_cb_001, test_str.c_str(), size_t(test_str.size()));
			//ct2.send(user_data, client_cb_001, test_str.c_str(), size_t(test_str.size()));
			//ct2.send(user_data, client_cb_001, test_str.c_str(), size_t(test_str.size()));
		}
	}
*/
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
	//	tmp_client->send(user_data, client_cb_001, test_str.c_str(), size_t(test_str.size()));
	}
	while (1)
	{
		//getchar();
		std::this_thread::sleep_for(std::chrono::milliseconds(10));

		for (int i = 0; i < 10; i++)
		{
			for (auto tmp_client : client_vector)
			{
				tmp_client->send(user_data, client_cb_001, test_str.c_str(), size_t(test_str.size()));
			}
		}
	}


	getchar();
	return 0;
}
