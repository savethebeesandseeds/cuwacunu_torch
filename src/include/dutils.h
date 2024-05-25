#pragma once
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <stdexcept>  // Include this at the top if not already included

#include "cuwacunu_config/config.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define FOR_ALL(arr, elem) for(auto& elem : arr)

extern pthread_mutex_t log_mutex;

/* This log functionality checks if there is a pendding log for the error trasported by the errno.h lib, 
 * WARNING! This functionaly sets the errno=0 if the errno is futher required, this might be not a desired behaviour.
 */
#define wrap_log_sys_err() {\
  if(errno != 0) {\
    pthread_mutex_lock(&log_mutex);\
    fprintf(LOG_ERR_FILE,"[%s0x%lX%s]: %sSYS ERRNO%s: ",\
      ANSI_COLOR_Cyan,pthread_self(),ANSI_COLOR_RESET,\
      ANSI_COLOR_ERROR,ANSI_COLOR_RESET);\
    fprintf(LOG_ERR_FILE,"[%d] %s\n", errno, strerror(errno));\
    fflush(LOG_ERR_FILE);\
    pthread_mutex_unlock(&log_mutex);\
    errno = 0;\
  }\
}
/* This log functionality marks the thread id, so that log messages are linked to the request thread */
#define log_info(...) {\
  wrap_log_sys_err();\
  pthread_mutex_lock(&log_mutex);\
  fprintf(LOG_FILE,"[%s0x%lX%s]: ",\
    ANSI_COLOR_Cyan,pthread_self(),ANSI_COLOR_RESET);\
  fprintf(LOG_FILE,__VA_ARGS__);\
  fflush(LOG_FILE);\
  pthread_mutex_unlock(&log_mutex);\
}
/* This log functionality marks the thread id, so that log messages are linked to the request thread */
#define log_dbg(...) {\
  wrap_log_sys_err();\
  pthread_mutex_lock(&log_mutex);\
  fprintf(LOG_ERR_FILE,"[%s0x%lX%s]: %sDEBUG%s: ",\
    ANSI_COLOR_Cyan,pthread_self(),ANSI_COLOR_RESET,\
    ANSI_COLOR_Bright_Blue,ANSI_COLOR_RESET);\
  fprintf(LOG_ERR_FILE,__VA_ARGS__);\
  fflush(LOG_ERR_FILE);\
  pthread_mutex_unlock(&log_mutex);\
}
/* This log functionality marks the thread id, so that log messages are linked to the request thread */
#define log_err(...) {\
  wrap_log_sys_err();\
  pthread_mutex_lock(&log_mutex);\
  fprintf(LOG_ERR_FILE,"[%s0x%lX%s]: %sERROR%s: ",\
    ANSI_COLOR_Cyan,pthread_self(),ANSI_COLOR_RESET,\
    ANSI_COLOR_ERROR,ANSI_COLOR_RESET);\
  fprintf(LOG_ERR_FILE,__VA_ARGS__);\
  fflush(LOG_ERR_FILE);\
  pthread_mutex_unlock(&log_mutex);\
}
/* This log functionality marks the thread id, so that log messages are linked to the request thread */
#define log_fatal(...) {\
  wrap_log_sys_err();\
  pthread_mutex_lock(&log_mutex);\
  fprintf(LOG_ERR_FILE,"[%s0x%lX%s]: %sFATAL%s: ",\
    ANSI_COLOR_Cyan,pthread_self(),ANSI_COLOR_RESET,\
    ANSI_COLOR_ERROR,ANSI_COLOR_RESET);\
  fprintf(LOG_ERR_FILE,__VA_ARGS__);\
  fflush(LOG_ERR_FILE);\
  pthread_mutex_unlock(&log_mutex);\
  THROW_RUNTIME_ERROR();\
}
/* This log functionality marks the thread id, so that log messages are linked to the request thread */
#define log_warn(...) {\
  wrap_log_sys_err();\
  pthread_mutex_lock(&log_mutex);\
  fprintf(LOG_WARN_FILE,"[%s0x%lX%s]: %sWARNING%s: ",\
    ANSI_COLOR_Cyan,pthread_self(),ANSI_COLOR_RESET,\
    ANSI_COLOR_WARNING,ANSI_COLOR_RESET);\
  fprintf(LOG_WARN_FILE,__VA_ARGS__);\
  fflush(LOG_WARN_FILE);\
  pthread_mutex_unlock(&log_mutex);\
}
/* --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- */
/* utilities */
#define STRINGIFY(x) #x
#define CONCAT_INTERNAL(x, y) x##y
#define CONCAT(x, y) CONCAT_INTERNAL(x, y)
/* compile-time warnings and errors */
#define COMPILE_TIME_WARNING(msg) _Pragma(STRINGIFY(GCC warning #msg))
#define THROW_COMPILE_TIME_ERROR(msg) _Pragma(STRINGIFY(GCC error #msg))
/* run-time warnings and errors */
struct RuntimeWarning { RuntimeWarning(const char *msg) { log_warn(msg); }};
#define RUNTIME_WARNING(msg) static RuntimeWarning CONCAT(rw_, __COUNTER__) (msg)
#define THROW_RUNTIME_ERROR() { throw std::runtime_error("Runtime error occurred"); }
/* --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- */