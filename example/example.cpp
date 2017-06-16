
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
server_base st1;
std::mutex mtx;
//std::lock_guard<std::mutex> lock(mtx);
void client_cb_001(const char *msg, size_t len, void *usr_data)
{
    std::cout << "receive message form server : \"" << (std::string(msg, len)) << "\" with user data : " << usr_data << std::endl;
}
void server_cb_001(const char *data, size_t len, void *ID)
{
    std::cout << "receive message form client : " << (std::string(data, len)) << std::endl;
    st1.send(data, len, ID);

    /*
    //    std::cout << "in server_cb" << std::endl;
    zmsg::ustring tmp = input.pop_front();
    std::cout << "identity is : ";
    for (auto i : tmp)
    {
        std::cout << std::to_string(int(i)) << " ";
    }
    std::cout << std::endl;
    //input.dump();

    std::cout << (char *)(input.pop_front().c_str()) << std::endl;
    
    std::string tmp_str("test from server");
    input.append(tmp_str.c_str());
    //input.body_set();
    st1.send(input);
    */
}
int main(void)
{
    client_base ct1;
    ct1.setIPPort("127.0.0.1:5570");
    client_base ct2;
    ct2.setIPPort("127.0.0.1:5570");

    st1.setIPPort("127.0.0.1:5570");
    st1.set_cb(server_cb_001);

    st1.run();
    ct1.run();
    ct2.run();

    std::string test_str = "this is for test!";
    void *user_data = (void *)28;
    std::cout << "send message : \"" << test_str << "\"  with usr data : " << user_data << std::endl;

    ct1.send(user_data, client_cb_001, test_str.c_str(), size_t(test_str.size()));
    ct2.send(user_data, client_cb_001, test_str.c_str(), size_t(test_str.size()));

    getchar();

    return 0;
}
