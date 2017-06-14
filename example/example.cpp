
#include "client_base.hpp"
#include "server_base.hpp"
#include "util.hpp"
#include <string>
#include <thread> // std::this_thread::sleep_for
#include <chrono> // std::chrono::seconds
#include <mutex>
#include <future>

void callback_f(zmsg &input)
{
    input.dump();
}
std::mutex mtx;
//std::lock_guard<std::mutex> lock(mtx);

int main(void)
{
    client_base ct1;
    ct1.setIPPort("tcp://127.0.0.1:5570");
    ct1.set_cb(callback_f);
    client_base ct2;
    ct2.setIPPort("tcp://127.0.0.1:5570");
    ct2.set_cb(callback_f);

    server_base st1;
    st1.set_cb(server_cb);

    st1.run();
    ct1.run();
    ct2.run();

    std::string test_str = "this is for test!";
    ct1.send(test_str.c_str(), test_str.size());
    ct2.send(test_str.c_str(), test_str.size());

    getchar();

    return 0;
}
