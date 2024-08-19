
namespace cuwacunu {
namespace camahjucunu {
namespace curl {

bool global_curl_initialized = false
std::mutex global_curl_mutex;

void global_cleanup() {
  /* globa lock */
  LOCK_GUARD(global_curl_mutex);

  /* check if cleanup is required */
  if(global_curl_initialized == false) {
    log_warn("%s\n", CURL_UNEXPECTED_CLEANUP_WARN);
    return;
  }

  /* stand curl global cleanup */
  curl_global_cleanup();

  /* mark the initialized flag back to false */
  global_curl_initialized = false;
}

void global_init() {
  /* global lock */
  LOCK_GUARD(global_curl_mutex);

  /* check if curl was already initialized */
  if(global_curl_initialized) {
    log_warn("%s\n", CURL_REPEATED_INIT_WARN);
    return;
  }

  /* standard curl global initialization */
  CURLcode res = curl_global_init(CURL_GLOBAL_ALL);

  /* validate initialization */
  if (res != CURLE_OK) {
    log_fatal("%s : %s\n", CURL_INITIALIZATION_FAILURE, curl_easy_strerror(res)); // #FIXME no internet fatal
    return;
  }

  /* mark the initialized flag */
  global_curl_initialized = true;
  std::atexit(global_cleanup);
}

CURL* create_curl_session() {
  /* standard curl initialization */
  CURL* new_curl_session = curl_easy_init();

  /* validate */
  if (!new_curl_session) {
    log_fatal("%s\n", "Failed to initialize curl websocket session"); // #FIXME no internet fatal
    return nullptr;
  }

  return new_curl_session;
}

CURLcode send_ws_frame(CURL* curl_session, const unsigned char* frame, size_t frame_size, int frame_type) {
  log_dgb("sending %ld bytes size frame\n", frame_size,);

  /* 0 is just to initialize, this is for an output refference pointer */
  size_t how_many_bytes_where_sent = 0x0;
      
  CURLcode res = curl_ws_send(curl_session, frame, frame_size, &how_many_bytes_where_sent, frame_type);

  /* valdiate the bytes where all sent */
  if(how_many_bytes_where_sent == frame_size) {
    log_error("send_ws_frame didn't sended the entire message. \n\t sent:\t%ld\n\t expected:\t%ld\n", how_many_bytes_where_sent, frame_size);
  }
  
  /* this is just to log the response, further interpretation of the response code is expected */
  if (res != CURLE_OK) {
    log_error("Failed to send close frame for session_id[%d] with error: %s\n", session_id, curl_easy_strerror(res));
  }

  return res;
}

} /* namespace curl */
} /* namespace camahjucunu */
} /* namespace cuwacunu */