#ifndef _BETAVITA_LOGGER_H
#define _BETAVITA_LOGGER_H

#include <cstdio>
#include <cstdint>

#include <string>

#define LOG_DEBUG(logger, ...) logger.log_debug(__FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(logger, ...) logger.log_error(__FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define LOG_INFO(logger, ...) logger.log_info(__FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define LOG_WARN(logger, ...) logger.log_warn(__FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)

struct Logger {
private:
    std::string name;
    FILE *file;
private:
    void normal_print(const char *filename, const char *function, int line, const char *level_name, const char *fmt, va_list ap);
public:
    Logger(const std::string& name = "emulator", FILE *f = stdout);
    void set_file_output(FILE *file);
    void set_logger_name(const std::string& logger_name);
    void log_info(const char *filename, const char *function, int line, const char *fmt, ...);
    void log_warn(const char *filename, const char *function, int line, const char *fmt, ...);
    void log_error(const char *filename, const char *function, int line, const char *fmt, ...);
    void log_debug(const char *filename, const char *function, int line, const char *fmt, ...);
};

extern Logger SELFLOADER, HLE, MEMORY, KERNEL;

#endif /* _BETAVITA_LOGGER_H */