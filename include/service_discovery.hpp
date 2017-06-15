#pragma once
#include <string>
#include <memory>
#include <util.hpp>
struct HostType
{
  enum type
  {
    DBW_IPV4,
    DBW_IPV6,
    DBW_FQDN,
    DBW_MAX
  };

  static std::string toString(const HostType::type _type)
  {
    return (_type >= DBW_MAX || _type < DBW_IPV4) ? "UNDEFINED_HOSTTYPE" : (const std::string[]){
                                                                               "DBW_IPV4",
                                                                               "DBW_IPV6",
                                                                               "DBW_FQDN"}[_type];
  }
};
struct HostAndPort
{
  std::string host;
  int port;

  HostAndPort() : host(""), port(0) {}
  HostAndPort(std::string _host, int _port) : host(_host), port(_port) {}

  bool operator==(const HostAndPort &_hostPort) const
  {
    return ((this->host == _hostPort.host) && (this->port == _hostPort.port));
  }

  bool operator<(const HostAndPort &_hostPort) const
  {
    if (this->host < _hostPort.host)
      return true;
    else if (this->host > _hostPort.host)
      return false;

    if (this->port < _hostPort.port)
      return true;
    else if (this->port > _hostPort.port)
      return false;

    return false;
  }

  bool isEmpty()
  {
    if ((host == "") && (port == 0))
      return true;
    else
      return false;
  }

  void clear()
  {
    host = "";
    port = 0;
  }
};

// Address type
struct DwAddrType
{
  enum type
  {
    INTERNAL_ADDR,    // internal address
    EXTERNAL_ADDR,    // external address
    SOURCE_ADDR,      // source address
    DESTINATION_ADDR, // destination address
  };

  static std::string toString(const DwAddrType::type type)
  {
    return (type > DESTINATION_ADDR || type < INTERNAL_ADDR) ? "UNDEFINED_ADDR_TYPE" : (const std::string[]){"INTERNAL_ADDR", "EXTERNAL_ADDR", "SOURCE_ADDR", "DESTINATION_ADDR"}[type];
  }
};

// Service Discovery Data
class service_discovery_interface
{
public:
  typedef std::shared_ptr<service_discovery_interface> ptr_t;
  virtual ~service_discovery_interface() {}
  inline static ptr_t create() { return ptr_t(new service_discovery_interface()); }
  inline static ptr_t create(std::string _svcLabel, int _dataType)
  {
    return ptr_t(new service_discovery_interface(_svcLabel, _dataType));
  }

  // set/get host and port
  virtual HostAndPort getHostAndPort(DwAddrType::type type, HostType::type hostType = HostType::DBW_MAX);
  virtual void setHostAndPort(HostAndPort &data, DwAddrType::type type);

  // set/get host type
  virtual HostType::type getHostType(DwAddrType::type type, HostType::type hostType = HostType::DBW_MAX);
  virtual void setHostType(HostType::type type, DwAddrType::type addrType);

  // check host type(especial for multiple addresses)
  virtual bool hasHostType(DwAddrType::type type, HostType::type hostType);

  // set/get data type
  virtual int getDataType(void) { return dataType; }
  virtual void setDataType(int type) { dataType = type; }

  // set/get service label
  virtual std::string getSvcLabel(void) { return svcLabel; }
  virtual void setSvcLabel(std::string label) { svcLabel = label; }

  virtual void dump(std::string prefix = "", bool onScreen = true);

  bool operator==(service_discovery_interface &data);
  bool operator<(service_discovery_interface &data);
  virtual bool less(service_discovery_interface::ptr_t lhs, service_discovery_interface::ptr_t rhs);

protected:
  service_discovery_interface() : svcLabel(""), dataType(0) {}
  service_discovery_interface(std::string _svcLabel, int _dataType)
  {
    svcLabel = _svcLabel;
    dataType = _dataType;
  }

  std::string svcLabel;
  int dataType;
};

class cleint_service_discovery : public service_discovery_interface
{
public:
};
class server_service_discovery : public service_discovery_interface
{
public:
  /*
  typedef std::shared_ptr<server_service_discovery> ptr_t;
  virtual ~server_service_discovery() {}
  inline static ptr_t create() { return ptr_t(new server_service_discovery()); }
  inline static ptr_t create(std::string _svcLabel, int _dataType)
  {
    return ptr_t(new server_service_discovery(_svcLabel, _dataType));
  }
  */
};