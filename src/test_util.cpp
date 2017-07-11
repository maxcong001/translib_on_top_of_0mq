#include "test_util.hpp"

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

    std::cout << std::dec << "+++ " << str_buff << " [" << Logger::logLevelString(lev) << "]: file: " << file_ptr << ", line: "
              << line << ", func: " << func_ptr << "\n"
              << msg << std::endl;
    return;
}

void client_cb_001(const char *msg, size_t len, void *usr_data)
{
    //std::cout << "receive message in the client callback, message is :" << std::string(msg, len) << "size is " << len << std::endl;
    std::cout
        << std::dec << " total message: " << message_count_recv++ << " user data is :" << (long)usr_data << std::endl; //"\r" << std::flush;
}

void client_monitor_func(int event, int value, std::string &address)
{
    std::cout << std::dec << "receive event form client monitor task, the event is " << event << ". Value is : " << value << ". string is : " << address << "string length is : " << address.size() << std::endl;
}
void server_monitor_func(int event, int value, std::string &address)
{
    std::cout << std::dec << "receive event form server monitor task, the event is " << event << ". Value is : " << value << ". string is : " << address << std::endl;
}
