#include "logger.hpp"

void CallbackLogger::debug(const char *file, int line, const char *func, const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  this->logFmtHandler(file, line, func, DEBUG, fmt, args);
  va_end(args);
}

void CallbackLogger::info(const char *file, int line, const char *func, const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  this->logFmtHandler(file, line, func, INFO, fmt, args);
  va_end(args);
}

void CallbackLogger::warn(const char *file, int line, const char *func, const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  this->logFmtHandler(file, line, func, WARN, fmt, args);
  va_end(args);
}

void CallbackLogger::error(const char *file, int line, const char *func, const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  this->logFmtHandler(file, line, func, ERROR, fmt, args);
  va_end(args);
}

void CallbackLogger::fatal(const char *file, int line, const char *func, const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  this->logFmtHandler(file, line, func, FATAL, fmt, args);
  va_end(args);
}

void CallbackLogger::logFmtHandler(const char *file, int line, const char *func, Logger::Level lev, const char *fmt, va_list args)
{
  if (!this->isValidHandler() || this->getLevel() > lev)
  {
    return;
  }
  char msg[LOG_BUFFER_SIZE];
  msg[0] = '\0';
  vsnprintf(msg, LOG_BUFFER_SIZE, fmt, args);
  this->logHandler(file, line, func, lev, msg);
  return;
}
