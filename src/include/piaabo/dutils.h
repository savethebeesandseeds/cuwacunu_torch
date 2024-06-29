#pragma once
#include <stdexcept>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <cctype>
#include <vector>
#include <sstream>
#include <string>
#include <iostream>
#include <iomanip>

#define LOG_FILE stdout
#define LOG_ERR_FILE stderr
#define LOG_WARN_FILE stdout

#define ANSI_COLOR_ERROR "\x1b[41m" 
#define ANSI_COLOR_SUCCESS "\x1b[42m" 
#define ANSI_COLOR_WARNING "\x1b[43m" 
#define ANSI_COLOR_RESET "\x1b[0m" 
#define ANSI_COLOR_Black "\x1b[30m" 
#define ANSI_COLOR_Red "\x1b[31m"     
#define ANSI_COLOR_Green "\x1b[32m"   
#define ANSI_COLOR_Yellow "\x1b[33m" 
#define ANSI_COLOR_Blue "\x1b[34m"  
#define ANSI_COLOR_Magenta "\x1b[35m" 
#define ANSI_COLOR_Cyan "\x1b[36m"    
#define ANSI_COLOR_White "\x1b[37m" 
#define ANSI_COLOR_Bright_Black_Grey "\x1b[90m"
#define ANSI_COLOR_Bright_Red "\x1b[91m" 
#define ANSI_COLOR_Bright_Green "\x1b[92m" 
#define ANSI_COLOR_Bright_Yellow "\x1b[93m"     
#define ANSI_COLOR_Bright_Blue "\x1b[94m" 
#define ANSI_COLOR_Bright_Magenta "\x1b[95m" 
#define ANSI_COLOR_Bright_Cyan "\x1b[96m"       
#define ANSI_COLOR_Bright_White "\x1b[97m"

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
namespace cuwacunu {
namespace piaabo {
std::string trim_string(const std::string& str);
std::vector<std::string> split_string(const std::string& str, char delimiter);
std::string to_hex_string(const unsigned char* data, size_t size);
} /* namespace cuwacunu */
} /* namespace piaabo */