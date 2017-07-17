/*
 * Copyright (c) 2016-20017 Max Cong <savagecm@qq.com>
 * this code can be found at https://github.com/maxcong001/translib_on_top_of_0mq
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
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