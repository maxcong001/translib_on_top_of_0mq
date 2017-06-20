#include <string>
#include <functional>
#include <mutex>
#include <map>
#include <cstdarg>
#include <iostream>

#define ZMQ_LOG __FILE__, __LINE__, __func__
#define logger LogManager::_logger

const size_t LOG_BUFFER_SIZE = 10240;

class Logger;
class SimpleLogger;

class Logger
{
public:
  enum Level
  {
    ALL,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL,
    OFF
  };

  void setLevel(Level lev) { this->level = lev; }
  Level getLevel() { return this->level; }

  static inline std::string logLevelString(const Level lev)
  {
    return (lev < ALL || lev > OFF) ? "UNDEFINED_LOGLEVEL" : (const std::string[]){
                                                                 "ALL",
                                                                 "DEBUG",
                                                                 "INFO",
                                                                 "WARN",
                                                                 "ERROR",
                                                                 "FATAL",
                                                                 "OFF"}[lev];
  }

  virtual void debug(const char *file, int line, const char *func, const char *fmt, ...) {}
  virtual void info(const char *file, int line, const char *func, const char *fmt, ...) {}
  virtual void warn(const char *file, int line, const char *func, const char *fmt, ...) {}
  virtual void error(const char *file, int line, const char *func, const char *fmt, ...) {}
  virtual void fatal(const char *file, int line, const char *func, const char *fmt, ...) {}

  static std::string convert(const std::map<std::string, std::string> &_map);

  virtual ~Logger() {}

protected:
  Logger() : level(ERROR) {}
  Logger(Level lev) : level(lev) {}

private:
  Level level;
};

class LogManager
{
public:
  // typedef std::function<void (const char *file, int line, const char *func, Logger::Level lev, const char *fmt, va_list args)> LogHandlerFn;
  typedef std::function<void(const char *file, int line, const char *func, Logger::Level lev, const char *msg)> LogHandlerFn;
  typedef std::lock_guard<std::mutex> Lock;

  static Logger *_logger;

  static Logger *getLogger();
  static Logger *getLogger(const LogHandlerFn fn);

  static void setLogHandler(const LogHandlerFn fn);

  static void destroy();

  ~LogManager() {}

private:
  static std::mutex mutex;
  LogManager();
};

class SimpleLogger : virtual public Logger
{
public:
  SimpleLogger() : Logger(), buf_size(LOG_BUFFER_SIZE) {}
  SimpleLogger(Level lev) : Logger(lev), buf_size(LOG_BUFFER_SIZE) {}

  virtual void debug(const char *file, int line, const char *func, const char *fmt, ...);
  virtual void info(const char *file, int line, const char *func, const char *fmt, ...);
  virtual void warn(const char *file, int line, const char *func, const char *fmt, ...);
  virtual void error(const char *file, int line, const char *func, const char *fmt, ...);
  virtual void fatal(const char *file, int line, const char *func, const char *fmt, ...);

  virtual ~SimpleLogger() {}

private:
  const size_t buf_size;
  size_t getBufSize() { return this->buf_size; }
  void simpleLogHandler(const char *file, int line, const char *func, Level lev, const char *fmt, va_list args);
};

class CallbackLogger : virtual public Logger
{
public:
  CallbackLogger() : Logger() {}
  CallbackLogger(Level lev) : Logger(lev) {}
  CallbackLogger(const LogManager::LogHandlerFn fn) : Logger(), logHandler(fn) {}
  CallbackLogger(const LogManager::LogHandlerFn fn, Level lev) : Logger(lev), logHandler(fn) {}

  virtual void debug(const char *file, int line, const char *func, const char *fmt, ...);
  virtual void info(const char *file, int line, const char *func, const char *fmt, ...);
  virtual void warn(const char *file, int line, const char *func, const char *fmt, ...);
  virtual void error(const char *file, int line, const char *func, const char *fmt, ...);
  virtual void fatal(const char *file, int line, const char *func, const char *fmt, ...);

  void setLogHandler(const LogManager::LogHandlerFn fn) { this->logHandler = fn; }

  virtual ~CallbackLogger() {}

private:
  LogManager::LogHandlerFn logHandler;

  bool isValidHandler()
  {
    if (this->logHandler)
    {
      return true;
    }
    else
    {
      return false;
    }
  }
  void logFmtHandler(const char *file, int line, const char *func, Logger::Level lev, const char *fmt, va_list args);
};
