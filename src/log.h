#ifndef __drool_log_h
#define __drool_log_h

#include <sys/types.h>

#define LOG_SETTINGS_T_INIT         { 0, 0, 0, 1, 1, 1 }
#define LOG_SETTINGS_T_INIT_NONE    { 0, 0, 0, 0, 0, 0 }
#define LOG_SETTINGS_T_INIT_ALL     { 1, 1, 1, 1, 1, 1 }
typedef struct log_settings log_settings_t;
struct log_settings {
    unsigned short      debug : 1;
    unsigned short      info : 1;
    unsigned short      notice : 1;
    unsigned short      warning : 1;
    unsigned short      error : 1;
    unsigned short      critical : 1;
};

#define LOG_LEVEL_UNKNOWN_STR   "unknown"
#define LOG_LEVEL_DEBUG_STR     "debug"
#define LOG_LEVEL_INFO_STR      "info"
#define LOG_LEVEL_NOTICE_STR    "notice"
#define LOG_LEVEL_WARNING_STR   "warning"
#define LOG_LEVEL_ERROR_STR     "error"
#define LOG_LEVEL_CRITICAL_STR  "critical"
typedef enum log_level log_level_t;
enum log_level {
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_NOTICE,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_CRITICAL,

    LOG_LEVEL_ALL
};
#define LDEBUG      LOG_LEVEL_DEBUG
#define LINFO       LOG_LEVEL_INFO
#define LNOTICE     LOG_LEVEL_NOTICE
#define LWARNING    LOG_LEVEL_WARNING
#define LERROR      LOG_LEVEL_ERROR
#define LCRITICAL   LOG_LEVEL_CRITICAL

int log_level_enable(log_settings_t* settings, const log_level_t level);
int log_level_disable(log_settings_t* settings, const log_level_t level);

#define LOG_FACILITY_UNKNOWN_STR    "unknown"
#define LOG_FACILITY_NONE_STR       "none"
#define LOG_FACILITY_CORE_STR       "core"
#define LOG_FACILITY_NETWORK_STR    "network"
typedef enum log_facility log_facility_t;
enum log_facility {
    LOG_FACILITY_NONE = 0,
    LOG_FACILITY_CORE,
    LOG_FACILITY_NETWORK
};
#define LNONE       LOG_FACILITY_NONE
#define LCORE       LOG_FACILITY_CORE
#define LNETWORK    LOG_FACILITY_NETWORK

#define LOG_T_INIT { \
    LOG_SETTINGS_T_INIT_ALL, \
    LOG_SETTINGS_T_INIT, \
    LOG_SETTINGS_T_INIT, \
}
typedef struct log log_t;
struct log {
    log_settings_t  none;
    log_settings_t  core;
    log_settings_t  network;
};

int log_enable(log_t* log, const log_facility_t facility, const log_level_t level);
int log_disable(log_t* log, const log_facility_t facility, const log_level_t level);

void log_printf_fileline(const log_t* log, const log_facility_t facility, const log_level_t level, const char* file, size_t line, const char* format, ...);
#define log_print(log, facility, level, text) \
    log_printf_fileline(log, facility, level, __FILE__, __LINE__, text)
#define log_printf(log, facility, level, format, args...) \
    log_printf_fileline(log, facility, level, __FILE__, __LINE__, format, args)

#endif /* __drool_log_h */
