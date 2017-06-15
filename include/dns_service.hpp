#pragma once
#include <functional>
#include <chrono>
#include <list>
#include <mutex>
#include <condition_variable>
class dns_service_interface
{
public:
  typedef std::function<void(std::list<std::string> &)> DNS_CB_FUNC_SERVER;
  typedef std::function<void(std::string)> DNS_CB_FUNC_APP;
  bool set_dns_cb(DNS_CB_FUNC_SERVER cb)
  {
    if (cb)
    {
      dns_cb_server = cb;
      return true;
    }
    else
    {
      return false;
    }
  }
  DNS_CB_FUNC_SERVER get_dns_cb()
  {
    return dns_cb_server;
  }

  bool getIPList()
  {
    dns_cb_app(remote_fqdn);

    std::unique_lock<std::mutex> dnsLock(dnsMutex);
    if (dnsCond.wait_for(dnsLock, std::chrono::seconds(5)) == std::cv_status::timeout)
    {
      if (IpList.empty())
      {
        return false;
      }
      (IpList);
    }
    else
    {
      return false;
    }
  }
  void setIPList(std::list<std::string> list)
  {
    IpList = list;
  }

  bool set_remote_fqdn(std::string fqdn)
  {
    remote_fqdn = fqdn;
    return true;
  }
  std::string get_remote_fqdn()
  {
    return remote_fqdn;
  }

private:
  // this is the function which server register to DNS service.
  // when IPlist come back, should call this function
  DNS_CB_FUNC_SERVER dns_cb_server;
  // this function is provide by application
  // the input is FQND string.
  DNS_CB_FUNC_APP dns_cb_app;
  std::list<std::string> IpList;
  std::string remote_fqdn;

  std::mutex dnsMutex;
  std::condition_variable dnsCond;
};