#include "util.hpp"
#include <iostream>
#include <string>
#include <memory>
void client_cb(zmsg &input)
{
    input.dump();
}
void server_cb(zmsg &input)
{
    //    std::cout << "in server_cb" << std::endl;
    zmsg::ustring tmp = input.pop_front();
    //zmsg *tmp = new zmsg(input);
    std::shared_ptr<zmsg> tmp_p(new zmsg(input));
    std::cout << "identity is : ";
    for (auto i : tmp)
    {
        std::cout << std::to_string(int(i)) << " ";
    }
    std::cout << std::endl;
    tmp_p->dump();

    std::cout << (char *)(input.pop_front().c_str()) << std::endl;
}