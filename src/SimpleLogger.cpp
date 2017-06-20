#include "logger.hpp"

void SimpleLogger::debug(const char *file, int line, const char *func, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    this->simpleLogHandler(file, line, func, DEBUG, fmt, args);
    va_end(args);
}

void SimpleLogger::info(const char *file, int line, const char *func, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    this->simpleLogHandler(file, line, func, INFO, fmt, args);
    va_end(args);
}

void SimpleLogger::warn(const char *file, int line, const char *func, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    this->simpleLogHandler(file, line, func, WARN, fmt, args);
    va_end(args);
}

void SimpleLogger::error(const char *file, int line, const char *func, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    this->simpleLogHandler(file, line, func, ERROR, fmt, args);
    va_end(args);
}

void SimpleLogger::fatal(const char *file, int line, const char *func, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    this->simpleLogHandler(file, line, func, FATAL, fmt, args);
    va_end(args);
}

void SimpleLogger::simpleLogHandler(const char *file, int line, const char *func, Logger::Level lev, const char *fmt, va_list args)
{
    if (this->getLevel() > lev)
    {
        return;
    }
    char buf[this->getBufSize()];
    buf[0] = '\0';
    vsnprintf(buf, this->getBufSize(), fmt, args);
    fprintf(stderr, "[%s]: file: %s, line: %d, func: %s\n%s\n", this->logLevelString(lev).c_str(), file, line, func, buf);
    return;
}

