#include "service_discovery.hpp"
/*********************** Common ***********************/
/**
 * @brief Compare functions used for "sort" and "difference"
 * @param lhs: pointer reference to common data
 * @param rhs: pointer reference to common data
 * @return true if lhs < rhs, otherwise return false
 */
bool compareDiscoveryData(const service_discovery_interface::ptr_t &lhs, const service_discovery_interface::ptr_t &rhs)
{
  return lhs->less(lhs, rhs);
}

/*********************** Service Discovery Base Data ***********************/
/**
 * @brief get host and port
 * @param type: address type
 * @return host and port
 */
HostAndPort service_discovery_interface::getHostAndPort(DwAddrType::type type, HostType::type hostType)
{
  // error log
  return HostAndPort(); // return empty data
}

/**
 * @brief set host and port
 * @param data: host and port
 * @param type: address type
 * @return void
 */
void service_discovery_interface::setHostAndPort(HostAndPort &data, DwAddrType::type type)
{
  // error log

  return;
}

/**
 * @brief get host type
 * @param type: address type
 * @return host type
 */
HostType::type service_discovery_interface::getHostType(DwAddrType::type type, HostType::type hostType)
{
  // error log
  return HostType::DBW_MAX; // return an invalid value
}

/**
 * @brief set host type
 * @param type: host type
 * @param addrType: address type
 * @return host type
 */
void service_discovery_interface::setHostType(HostType::type type, DwAddrType::type addrType)
{
  // error log

  return;
}

/**
 * @brief check host type exists or not
 * @param addrType: address type
 * @param type: host type
 * @return true/false
 */
bool service_discovery_interface::hasHostType(DwAddrType::type addrType, HostType::type type)
{
  return (getHostType(addrType) == type);
}

/**
 * @brief operator == for service_discovery_interface
 * @param data: reference for common data
 * @return true if equal, otherwise return false
 */
bool service_discovery_interface::operator==(service_discovery_interface &data)
{
  bool flag = ((this->svcLabel == data.svcLabel) &&
               (this->dataType == data.dataType));

  return flag;
}

/**
 * @brief operator < for service_discovery_interface
 * @param data: reference for common data
 * @return true if *this < data, otherwise return false
 */
bool service_discovery_interface::operator<(service_discovery_interface &data)
{
  // compare based on priority: dataType > svcLabel
  if (this->dataType < data.dataType)
  {
    return true;
  }
  else if (this->dataType > data.dataType)
  {
    return false;
  }

  if (this->svcLabel < data.svcLabel)
  {
    return true;
  }
  else if (this->svcLabel > data.svcLabel)
  {
    return false;
  }

  return false;
}

/**
 * @brief Compare service_discovery_interface
 * @param lhs: pointer to common data
 * @param rhs: pointer to common data
 * @return true if *lhs < *rhs, otherwise return false
 */
bool service_discovery_interface::less(service_discovery_interface::ptr_t lhs, service_discovery_interface::ptr_t rhs)
{
  if (!lhs || !rhs)
  {
    return false;
  }

  return (*lhs < *rhs);
}

/**
 * @brief Dump service_discovery_interface
 * @param prefix: prefix used when dumping data
 * @param onScreen: indicating whether dump log on stdout
 * @return void
 */
void service_discovery_interface::dump(std::string prefix, bool onScreen)
{
  std::ostringstream os;
  os << prefix << "Dump service_discovery_interface(" << std::hex << this << ") ..." << std::endl;

  prefix += "\t";

  os << prefix << "dataType: " << dataType << ", svcLabel: " << svcLabel << std::endl;

  if (onScreen)
  {
    printf("%s\n", os.str().c_str());
  }
  else
  {
  }

  return;
}