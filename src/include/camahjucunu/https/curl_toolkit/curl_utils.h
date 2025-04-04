/* curl_utils.h */
#pragma once
#include <curl/curl.h>
#include <curl/curl.h>
#include <iostream>
#include <string>
#include <vector>
#include <mutex>
#include "piaabo/dutils.h"

#define CURL_REPEATED_INIT_WARN "Repeated WebsocketAPI::init(). Skipping and continuing as expected."
#define CURL_INITIALIZATION_FAILURE "Failed to initialize curl"
#define CURL_UNEXPECTED_CLEANUP_WARN "Request to WebsocketAPI::cleanup() without initializing. Skipping and continuing as expected."

#define NULL_CURL_SESSION -1

namespace cuwacunu {
namespace camahjucunu {
namespace curl {
extern std::mutex global_curl_mutex;
extern bool global_curl_initialized;

void dcurl_global_cleanup();
void dcurl_global_init();

CURL* create_curl_session();

CURLcode send_ws_frame(CURL* curl_session, const unsigned char* frame, size_t frame_size, int frame_type);

uint16_t ws_htons(uint16_t hostshort);

} /* namespace curl */
} /* namespace camahjucunu */
} /* namespace cuwacunu */