#include "logger.hpp"

std::mutex LogManager::mutex;
Logger *LogManager::_logger = LogManager::getLogger();

Logger *LogManager::getLogger()
{
  Lock lock(mutex);
  if (!_logger)
  {
    Logger *temp = new SimpleLogger();
    _logger = temp;
  }
  return _logger;
}

Logger *LogManager::getLogger(const LogManager::LogHandlerFn fn)
{
  Lock lock(mutex);
  if (!fn)
  {
    return _logger;
  }
  if (_logger && typeid(_logger) != typeid(CallbackLogger))
  {
    delete _logger;
    _logger = NULL;
  }
  if (!_logger)
  {
    Logger *temp = new CallbackLogger(fn);
    _logger = temp;
  }
  return _logger;
}

void LogManager::setLogHandler(const LogHandlerFn fn)
{
  logger = getLogger(fn);
}

void LogManager::destroy()
{
  Lock lock(mutex);
  if (_logger)
  {
    delete _logger;
    _logger = NULL;
  }
}