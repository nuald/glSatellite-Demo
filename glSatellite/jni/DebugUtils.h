#pragma once

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define AT __FILE__ ":" TOSTRING(__LINE__)

const size_t MAX_ERR_LEN = 255;

class RuntimeError {
    char error[MAX_ERR_LEN];
public:
    RuntimeError(const char *at, const char *format, ...) {
        char msg[MAX_ERR_LEN];
        va_list ap;
        va_start(ap, format);
        vsnprintf(msg, sizeof(msg), format, ap);
        va_end(ap);

        snprintf(error, sizeof(error), "Error at %s:\n%s", at, msg);
    }
    const char* what() const {
        return error;
    }
};
