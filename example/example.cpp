
#include "client_base.hpp"
#include "server_base.hpp"
#include "util.hpp"
#include <string>
#include <thread> // std::this_thread::sleep_for
#include <chrono> // std::chrono::seconds
#include <mutex>
#include <future>
server_base st1;
void callback_f(zmsg &input)
{
    input.dump();
}
std::mutex mtx;
//std::lock_guard<std::mutex> lock(mtx);
void server_cb1(zmsg &input)
{
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
    */
    std::string tmp_str("test from server");
    input.append(tmp_str.c_str());
    //input.body_set();
    st1.send(input);
}
int main(void)
{
    client_base ct1;
    ct1.setIPPort("127.0.0.1:5570");
    ct1.set_cb(callback_f);
    client_base ct2;
    ct2.setIPPort("127.0.0.1:5570");
    ct2.set_cb(callback_f);

        st1.set_cb(server_cb1);

    st1.run();
    ct1.run();
    ct2.run();

    std::string test_str = "this is for test!";
    ct1.send(test_str.c_str(), test_str.size());
    ct2.send(test_str.c_str(), test_str.size());

    getchar();

    return 0;
}
