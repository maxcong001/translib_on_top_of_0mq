
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
int message_count;

size_t time_str(uint32_t secs, uint32_t msec, char *out_ptr, size_t sz)
{
    size_t len;           // String length
    time_t lsecs;         // Local secs value
    struct tm local_time; // Local timestamp

    // Convert seconds to a time value
    // Convert secs to the 32/64 bit format for this client
    lsecs = (time_t)secs;
    (void)localtime_r(&lsecs, &local_time);
    len = snprintf(out_ptr, sz, "%4u/%02u/%02u %02u:%02u:%02u.%03u",
                   local_time.tm_year + 1900,
                   local_time.tm_mon + 1,
                   local_time.tm_mday,
                   local_time.tm_hour,
                   local_time.tm_min,
                   local_time.tm_sec,
                   msec);

    if (len >= sz)
    {
        *(out_ptr + sz - 1) = '\0';
        len = sz;
    }

    return (len);
}

void logging_cb(const char *file_ptr, int line, const char *func_ptr, Logger::Level lev, const char *msg)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);

    char str_buff[256 + 1] = "";
    time_str(tv.tv_sec, tv.tv_usec, (char *)(&str_buff), 256);

    std::cout << "+++ " << str_buff << " [" << Logger::logLevelString(lev) << "]: file: " << file_ptr << ", line: "
              << line << ", func: " << func_ptr << "\n"
              << msg << std::endl;
    return;
}

//std::lock_guard<std::mutex> lock(mtx);
void client_cb_001(const char *msg, size_t len, void *usr_data)
{
    std::cout << "receive message form server : \"" << (std::string(msg, len)) << "\" with user data : " << usr_data << std::endl;
}
void server_cb_001(const char *data, size_t len, void *ID)
{
    std::cout << "receive message form client : " << (std::string(data, len)) << " total message: " << message_count++ << std::endl;
    st1.send(data, len, ID);
}
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
    LogManager::getLogger(logging_cb)->setLevel(Logger::ALL);

    logger->error(ZMQ_LOG, "hello world\n");

    client_base ct1;
    ct1.set_monitor_cb(client_monitor_func);
    ct1.setIPPort("127.0.0.1:5571");
    //ct1.setIPPortSource("127.0.0.1:5578");

    client_base ct2;
    ct2.set_monitor_cb(client_monitor_func);
    ct2.setIPPort("127.0.0.1:5571");
    //    ct1.setIPPortSource("127.0.0.1:5577");

    st1.setIPPort("127.0.0.1:5571");
    st1.set_monitor_cb(server_monitor_func);
    // for server, you need to set callback function first
    st1.set_cb(server_cb_001);

    ct1.run();
    ct2.run();

    st1.run();

    std::string test_str = "this is for test!";
    void *user_data = (void *)28;
    std::cout << "send message : \"" << test_str << "\"  with usr data : " << user_data << std::endl;

    for (int i = 0; i < 10; i++)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(400));

        ct1.send(user_data, client_cb_001, test_str.c_str(), size_t(test_str.size()));
        //ct2.send(user_data, client_cb_001, test_str.c_str(), size_t(test_str.size()));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(4000));
    ct2.send(user_data, client_cb_001, test_str.c_str(), size_t(test_str.size()));
    ct1.stop();

    getchar();
    return 0;
}
