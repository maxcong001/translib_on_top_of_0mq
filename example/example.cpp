
#include"client_base.hpp"
#include"server_base.hpp"
int main(void)
{
    client_base ct1;
    //client_base ct2;
    server_base st1;
    st1.run();
    ct1.run();
    std::string test_str("this is a test");
    ct1.send(test_str.c_str(), test_str.size());

	sleep(5);

    return 0;
}
