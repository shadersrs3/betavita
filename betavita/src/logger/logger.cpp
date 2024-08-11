#include <cstdarg>

#include <logger/logger.h>

Logger::Logger(const std::string& _name, FILE *f) : name(_name), file(f) {}

void Logger::set_logger_name(const std::string& logger_name) {
    name = logger_name;
}

void Logger::set_file_output(FILE *_file) {
    file = _file;
}

void Logger::normal_print(const char *filename, const char *function, int line, const char *level_name, const char *fmt, va_list ap) {
    fprintf(file, "[%s:%d] %s (%s): ", function, line, level_name, name.c_str());
    vfprintf(file, fmt, ap);
    fputc('\n', file);
}

void Logger::log_info(const char *filename, const char *function, int line, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    normal_print(filename, function, line, "INFO", fmt, ap);
    va_end(ap);
}

void Logger::log_error(const char *filename, const char *function, int line, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    normal_print(filename, function, line, "ERROR", fmt, ap);
    va_end(ap);
}

void Logger::log_warn(const char *filename, const char *function, int line, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    normal_print(filename, function, line, "WARN", fmt, ap);
    va_end(ap);
}

void Logger::log_debug(const char *filename, const char *function, int line, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    normal_print(filename, function, line, "DEBUG", fmt, ap);
    va_end(ap);
}

void Logger::log_hle(const char *filename, const char *function, int line, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    normal_print(filename, function, line, "HLE", fmt, ap);
    va_end(ap);
}