
#include "client_base.hpp"
#include "server_base.hpp"
#include <string>
#include <thread> // std::this_thread::sleep_for
#include <chrono> // std::chrono::seconds
#include <mutex>
#include <future>
int seq_number;
int seq_number1;
std::mutex mtx;
void tmp_cb(char *input)
{
    std::string tmp_str(input);
    std::cout << "receive: " << tmp_str << std::endl;
}
void callback_f(char *input)
{
    //std::lock_guard<std::mutex> lock(mtx);
    std::string tmp_str(input);
    int tmp_num = std::stoi(tmp_str, nullptr, 0);
    if (tmp_num == seq_number)
    {
        std::cout << "pass, id :" << tmp_num << std::endl;
        //std::lock_guard<std::mutex> lock(mtx);
        seq_number++;
    }
    else
    {
        std::cout << "fail, tmp_num is : " << tmp_num << " seq_number is :" << seq_number << " , less : " << (tmp_num - seq_number) << std::endl;
        seq_number++;
    }
}
void callback_f1(char *input)
{
    //std::lock_guard<std::mutex> lock(mtx);
    std::string tmp_str(input);
    int tmp_num = std::stoi(tmp_str, nullptr, 0);
    if (tmp_num == seq_number1)
    {
        seq_number1++;
        std::cout << "pass1, id :" << tmp_num << std::endl;
    }
    else
    {
        std::cout << "fail, tmp_num1 is : " << tmp_num << " seq_number1 is :" << seq_number1 << " , less : " << (tmp_num - seq_number1) << std::endl;
        seq_number1++;
    }
}
int main(void)
{
    seq_number = 0;
    seq_number1 = 0;
    int send_seq = 0;
    int send_seq1 = 0;
    client_base ct1;
    ct1.set_cb(callback_f);
    client_base ct2;
    ct2.set_cb(callback_f1);

    std::vector<client_base *> client_vector;
    for (int num = 0; num < 20; num++)
    {
        client_vector.emplace_back(new client_base());
    }
    for (auto tmp_client : client_vector)
    {
        tmp_client->set_cb(tmp_cb);
        tmp_client->run();
        std::string tmp_str("test");
        tmp_client->send(tmp_str.c_str(), tmp_str.size());
    }

    //client_base ct2;
    server_base st1;
    st1.run();
    ct1.run();
    ct2.run();

    std::thread th1([&]() {
        for (int i = 0; i < 100000000; i++)
        {

            //std::this_thread::sleep_for(std::chrono::milliseconds(2));
            //std::lock_guard<std::mutex> lock(mtx);
            for (int j = 0; j < 1000; j++)
            {
                std::string test_str = std::to_string(send_seq);
                ct1.send(test_str.c_str(), test_str.size());
                send_seq++;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    });

    th1.detach();
    std::thread th2([&]() {
        for (int i = 0; i < 100000000; i++)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            std::string test_str1 = std::to_string(send_seq1);
            ct2.send(test_str1.c_str(), test_str1.size());
            send_seq1++;
        } });
    th2.detach();

    while (1)
    {
        for (auto tmp_client : client_vector)
        {
            std::string tmp_str("test!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
            tmp_client->send(tmp_str.c_str(), tmp_str.size());
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
/*
        if (!(i % 1001))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }*/

#if 0
// large message test
#define MAX_MSG_LEN 10 * 1024
    char large[MAX_MSG_LEN] = {0};
    for (int j = 0; j < MAX_MSG_LEN; j++)
    {
        large[j] = 'a';
        if (j == 0)
        {
            large[j] = 's';
        }
        if (j == 1)
        {
            large[j] = 's';
        }
        if (j == 100)
        {
            large[j] = 's';
        }
        if (j == 1000)
        {
            large[j] = 's';
        }
        if (j == 1000 * 9)
        {
            large[j] = 's';
        }

        if (j == MAX_MSG_LEN - 1)
        {
            large[j] = 'b';
        }
    }

    for (int i = 0; i < 1; i++)
    {
        std::string log_m(large, MAX_MSG_LEN);
        std::cout << "send out message : len is " << MAX_MSG_LEN << ", body is:" << log_m << std::endl;
        ct1.send(large, MAX_MSG_LEN);
    }
#endif
    //sleep(500);
    getchar();
    for (auto tmp_client : client_vector)
    {
        delete tmp_client;
    }
    return 0;
}
