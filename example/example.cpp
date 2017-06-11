
#include "client_base.hpp"
#include "server_base.hpp"
#include <string>
int main(void)
{
    client_base ct1;
    //client_base ct2;
    server_base st1;
    st1.run();
    ct1.run();
    std::string test_str("this is a test");
    for (int i = 0; i < 5; i++)
    {
        ct1.send(test_str.c_str(), test_str.size());
    }
// large message test
#define MAX_MSG_LEN 10 * 1024
    char large[MAX_MSG_LEN] = {0};
    for (int j = 0; j < MAX_MSG_LEN; j++)
    {
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
          if (j == 1000*9)
        {
            large[j] = 's';
        }                    
        large[j] = 'a';
        if (j == MAX_MSG_LEN - 1)
        {
            large[j] = 'b';
        }
    }

    for (int i = 0; i < 1; i++)
    {
        std::string log_m(large, MAX_MSG_LEN);
        std::cout <<"send out message : len is "<<MAX_MSG_LEN<<", body is:" <<log_m<<std::endl;
        ct1.send(large, MAX_MSG_LEN);
    }
    sleep(5);

    return 0;
}

