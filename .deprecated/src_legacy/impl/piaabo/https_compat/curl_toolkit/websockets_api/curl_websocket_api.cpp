/* curl_websocket_api.cpp */
#include "piaabo/https_compat/curl_toolkit/websockets_api/curl_websocket_api.h"

DEV_WARNING("(curl_websocket_api.cpp)[] fatal error on unknown session_id (fatal might be a good thing here there shodun't be a reason to allow undefined instructions).\n");
DEV_WARNING("(curl_websocket_api.cpp)[ws_incomming_data_t] not necesarly local_timestamp matches the timestamps in the body of the responces. \n");
DEV_WARNING("(curl_websocket_api.cpp)[ws_incomming_data_t] this implementation (on deques) relies on the ability of the server to retrive an \"id\" key on the data, to track which incommind frame correspond to which output frame. This can be changed for other types of WS interactions, for now, this implementation is tailored to interacti with binance or alike servers. \n");
DEV_WARNING("(curl_websocket_api.cpp)[] writing to dbg might be slow if dbg is checking config every time.\n");
DEV_WARNING("(curl_websocket_api.cpp)[] TX_deque for orders, might delay the sending of instructions, so include time_window in the instruction.\n");
DEV_WARNING("(curl_websocket_api.cpp)[] many curl options are uncommented, this needs to be reviewed.\n");
DEV_WARNING("(curl_websocket_api.cpp)[ws_write] add encoding support for ws_write_text.\n");
DEV_WARNING("(curl_websocket_api.cpp)[] log the times and redirection count (curl_easy_getinfo()).\n");
DEV_WARNING("(curl_websocket_api.cpp)[] split header into implementation file .cpp (maybe not).\n");
DEV_WARNING("(curl_websocket_api.cpp)[] fix the possible infinite waits.\n");
DEV_WARNING("(curl_websocket_api.cpp)[] CURLOPT_BUFFERSIZE has a maximun, breaking large data responses in chunks on websocket_RX_callback, a server might mix these responses, making it impossible to retrive the complete message (binance seems to respect this alright).\n");
DEV_WARNING("(curl_websocket_api.cpp)[] websocket_RX_callback expects data to be a valid json format (complete or separated in multiple chunks).\n");

namespace cuwacunu {
namespace piaabo {
namespace curl {

namespace {
constexpr std::size_t kWsMaxRxBufferBytes = 1u << 20; /* 1 MiB safety cap */
}

/* Static member variable definitions */
int WebsocketAPI::sessions_counter = 0;
std::mutex WebsocketAPI::global_ws_mutex;
std::unordered_map<ws_session_id_t, CURL*> WebsocketAPI::curl_ws_sessions;
std::unordered_map<ws_session_id_t, CURLM *> WebsocketAPI::curl_multi_handles;
std::unordered_map<ws_session_id_t, std::shared_ptr<int>> WebsocketAPI::curl_ws_session_still_running;
std::unordered_map<ws_session_id_t, std::shared_ptr<std::mutex>> WebsocketAPI::session_mutex;
std::unordered_map<ws_session_id_t, std::shared_ptr<std::thread>> WebsocketAPI::session_RX_thread;
std::unordered_map<ws_session_id_t, std::shared_ptr<std::thread>> WebsocketAPI::session_TX_thread;
std::unordered_map<ws_session_id_t, std::shared_ptr<std::deque<ws_incomming_data_t>>> WebsocketAPI::session_RX_frames_deque;
std::unordered_map<ws_session_id_t, std::shared_ptr<std::deque<ws_outgoing_data_t>>> WebsocketAPI::session_TX_frames_deque;
std::unordered_map<ws_session_id_t, std::shared_ptr<std::string>> WebsocketAPI::session_RX_buffer;
std::unordered_map<ws_session_id_t, std::shared_ptr<bool>> WebsocketAPI::session_shutdown_requested;
std::unordered_map<ws_session_id_t, std::shared_ptr<bool>> WebsocketAPI::session_handshake_ready;
std::unordered_map<ws_session_id_t, std::shared_ptr<ws_session_id_t>> WebsocketAPI::id_sessions;
std::unordered_map<ws_session_id_t, std::shared_ptr<std::condition_variable>> WebsocketAPI::session_triggers;

WebsocketAPI::_init WebsocketAPI::_initializer;

/* Enforce Singleton requirements */
WebsocketAPI::WebsocketAPI() = default;
WebsocketAPI::~WebsocketAPI() = default;

/* Access point Constructor */
WebsocketAPI::_init::_init() { WebsocketAPI::init(); }
void WebsocketAPI::init() {
  log_dbg("Initializing WebsocketAPI \n");

  /* Trigger a cleanup process at the end */
  std::atexit(WebsocketAPI::finit);

  {
    LOCK_GUARD(WebsocketAPI::global_ws_mutex);
    /* Make sure curl is globally initialized */
    dcurl_global_init();
  }
}
void WebsocketAPI::finit() {
  log_dbg("Finalizing WebsocketAPI \n");

  /* Gracefully finalize active sessions before global cleanup. */
  std::vector<ws_session_id_t> active_sessions;
  {
    LOCK_GUARD(WebsocketAPI::global_ws_mutex);
    active_sessions.reserve(WebsocketAPI::session_mutex.size());
    for (const auto& it : WebsocketAPI::session_mutex) {
      if (it.first != NULL_CURL_SESSION) active_sessions.push_back(it.first);
    }
  }
  for (const ws_session_id_t session_id : active_sessions) {
    bool exists = false;
    {
      LOCK_GUARD(WebsocketAPI::global_ws_mutex);
      exists = WebsocketAPI::session_mutex.find(session_id) !=
               WebsocketAPI::session_mutex.end();
    }
    if (!exists) continue;
    try {
      WebsocketAPI::ws_finalize(session_id);
    } catch (...) {
      log_err("WebsocketAPI::finit failed to finalize session_id[ %d ]\n",
              session_id);
    }
  }

  {
    LOCK_GUARD(WebsocketAPI::global_ws_mutex);
    dcurl_global_cleanup();
  }
}

/* Session utilities (private) */
CURL* WebsocketAPI::get_session(const ws_session_id_t session_id) {
  LOCK_GUARD(WebsocketAPI::global_ws_mutex);
  const auto it = curl_ws_sessions.find(session_id);
  if (session_id == NULL_CURL_SESSION || it == curl_ws_sessions.end()) {
    log_fatal("%s with session_id[ %d ]\n", "Failed to identify curl websocket session", session_id);
    return nullptr;
  }
  return it->second;
}
CURLM* WebsocketAPI::get_session_multi_handle(const ws_session_id_t session_id) {
  LOCK_GUARD(WebsocketAPI::global_ws_mutex);
  const auto it = WebsocketAPI::curl_multi_handles.find(session_id);
  if (session_id == NULL_CURL_SESSION || it == WebsocketAPI::curl_multi_handles.end()) {
    log_fatal("%s with session_id[ %d ]\n", "Failed to identify curl websocket multi handle", session_id);
    return nullptr;
  }
  return it->second;
}
std::shared_ptr<std::mutex> WebsocketAPI::get_session_mutex(const ws_session_id_t session_id) {
  LOCK_GUARD(WebsocketAPI::global_ws_mutex);
  const auto it = WebsocketAPI::session_mutex.find(session_id);
  if (session_id == NULL_CURL_SESSION || it == WebsocketAPI::session_mutex.end()) {
    log_fatal("%s with session_id[ %d ]\n", "Failed to identify websocket session mutex", session_id);
    return nullptr;
  }
  return it->second;
}
std::shared_ptr<std::condition_variable> WebsocketAPI::get_session_trigger(const ws_session_id_t session_id) {
  LOCK_GUARD(WebsocketAPI::global_ws_mutex);
  const auto it = WebsocketAPI::session_triggers.find(session_id);
  if (session_id == NULL_CURL_SESSION || it == WebsocketAPI::session_triggers.end()) {
    log_fatal("%s with session_id[ %d ]\n", "Failed to identify websocket session trigger", session_id);
    return nullptr;
  }
  return it->second;
}
std::shared_ptr<int> WebsocketAPI::get_session_still_running(const ws_session_id_t session_id) {
  LOCK_GUARD(WebsocketAPI::global_ws_mutex);
  const auto it = WebsocketAPI::curl_ws_session_still_running.find(session_id);
  if (session_id == NULL_CURL_SESSION || it == WebsocketAPI::curl_ws_session_still_running.end()) {
    log_fatal("%s with session_id[ %d ]\n", "Failed to identify websocket session running state", session_id);
    return nullptr;
  }
  return it->second;
}
std::shared_ptr<ws_session_id_t> WebsocketAPI::get_session_id_ref(const ws_session_id_t session_id) {
  LOCK_GUARD(WebsocketAPI::global_ws_mutex);
  const auto it = WebsocketAPI::id_sessions.find(session_id);
  if (session_id == NULL_CURL_SESSION || it == WebsocketAPI::id_sessions.end()) {
    log_fatal("%s with session_id[ %d ]\n", "Failed to identify websocket session id reference", session_id);
    return nullptr;
  }
  return it->second;
}
std::shared_ptr<bool> WebsocketAPI::get_session_shutdown_flag(const ws_session_id_t session_id) {
  LOCK_GUARD(WebsocketAPI::global_ws_mutex);
  const auto it = WebsocketAPI::session_shutdown_requested.find(session_id);
  if (session_id == NULL_CURL_SESSION || it == WebsocketAPI::session_shutdown_requested.end()) {
    log_fatal("%s with session_id[ %d ]\n", "Failed to identify websocket session shutdown flag", session_id);
    return nullptr;
  }
  return it->second;
}
std::shared_ptr<bool> WebsocketAPI::get_session_handshake_flag(const ws_session_id_t session_id) {
  LOCK_GUARD(WebsocketAPI::global_ws_mutex);
  const auto it = WebsocketAPI::session_handshake_ready.find(session_id);
  if (session_id == NULL_CURL_SESSION || it == WebsocketAPI::session_handshake_ready.end()) {
    log_fatal("%s with session_id[ %d ]\n", "Failed to identify websocket session handshake flag", session_id);
    return nullptr;
  }
  return it->second;
}
std::shared_ptr<std::thread> WebsocketAPI::get_session_RX_thread(const ws_session_id_t session_id) {
  LOCK_GUARD(WebsocketAPI::global_ws_mutex);
  const auto it = WebsocketAPI::session_RX_thread.find(session_id);
  if (session_id == NULL_CURL_SESSION || it == WebsocketAPI::session_RX_thread.end()) {
    log_fatal("%s with session_id[ %d ]\n", "Failed to identify websocket RX thread", session_id);
    return nullptr;
  }
  return it->second;
}
std::shared_ptr<std::thread> WebsocketAPI::get_session_TX_thread(const ws_session_id_t session_id) {
  LOCK_GUARD(WebsocketAPI::global_ws_mutex);
  const auto it = WebsocketAPI::session_TX_thread.find(session_id);
  if (session_id == NULL_CURL_SESSION || it == WebsocketAPI::session_TX_thread.end()) {
    log_fatal("%s with session_id[ %d ]\n", "Failed to identify websocket TX thread", session_id);
    return nullptr;
  }
  return it->second;
}
std::shared_ptr<std::string> WebsocketAPI::get_session_RX_buffer(const ws_session_id_t session_id) {
  LOCK_GUARD(WebsocketAPI::global_ws_mutex);
  const auto it = WebsocketAPI::session_RX_buffer.find(session_id);
  if (session_id == NULL_CURL_SESSION || it == WebsocketAPI::session_RX_buffer.end()) {
    log_fatal("%s with session_id[ %d ]\n", "Failed to identify websocket session RX buffer", session_id);
    return nullptr;
  }
  return it->second;
}
std::shared_ptr<std::deque<ws_incomming_data_t>> WebsocketAPI::get_session_RX_deque(const ws_session_id_t session_id) {
  LOCK_GUARD(WebsocketAPI::global_ws_mutex);
  const auto it = WebsocketAPI::session_RX_frames_deque.find(session_id);
  if (session_id == NULL_CURL_SESSION || it == WebsocketAPI::session_RX_frames_deque.end()) {
    log_fatal("%s with session_id[ %d ]\n", "Failed to identify websocket session RX deque", session_id);
    return nullptr;
  }
  return it->second;
}
std::shared_ptr<std::deque<ws_outgoing_data_t>> WebsocketAPI::get_session_TX_deque(const ws_session_id_t session_id) {
  LOCK_GUARD(WebsocketAPI::global_ws_mutex);
  const auto it = WebsocketAPI::session_TX_frames_deque.find(session_id);
  if (session_id == NULL_CURL_SESSION || it == WebsocketAPI::session_TX_frames_deque.end()) {
    log_fatal("%s with session_id[ %d ]\n", "Failed to identify websocket session TX deque", session_id);
    return nullptr;
  }
  return it->second;
}
void WebsocketAPI::remove_session(const ws_session_id_t session_id) {
  std::unique_lock<std::mutex> global_lock(WebsocketAPI::global_ws_mutex);
  if (session_id == NULL_CURL_SESSION) {
    log_warn("remove_session ignored NULL session_id[ %d ]\n", session_id);
    return;
  }
  auto mutex_it = WebsocketAPI::session_mutex.find(session_id);
  if (mutex_it == WebsocketAPI::session_mutex.end()) {
    return;
  }

  std::unique_lock<std::mutex> session_lock(*mutex_it->second);
  WebsocketAPI::curl_ws_sessions.erase(session_id);
  WebsocketAPI::curl_multi_handles.erase(session_id);
  WebsocketAPI::session_RX_frames_deque.erase(session_id);
  WebsocketAPI::session_RX_thread.erase(session_id);
  WebsocketAPI::session_TX_thread.erase(session_id);
  WebsocketAPI::curl_ws_session_still_running.erase(session_id);
  WebsocketAPI::session_triggers.erase(session_id);
  WebsocketAPI::session_RX_buffer.erase(session_id);
  WebsocketAPI::session_shutdown_requested.erase(session_id);
  WebsocketAPI::session_handshake_ready.erase(session_id);
  WebsocketAPI::id_sessions.erase(session_id);
  session_lock.unlock();
  WebsocketAPI::session_mutex.erase(mutex_it);
}

/* Session utils (private)
   - initialize session
   - disconnect session
*/
ws_session_id_t WebsocketAPI::initialize_curl_ws_session() {
  /* Initialize the session */
  CURL* new_curl_session = create_curl_session();
  
  /* Initialize the multi init object */
  CURLM * new_curl_multi_handle = curl_multi_init();
  if (new_curl_multi_handle == nullptr) {
    curl_easy_cleanup(new_curl_session);
    log_fatal("%s\n", "Failed to initialize curl websocket multi handle");
    return NULL_CURL_SESSION;
  }

  ws_session_id_t new_session_id = NULL_CURL_SESSION;
  {
    LOCK_GUARD(WebsocketAPI::global_ws_mutex);
    new_session_id = WebsocketAPI::sessions_counter++;
    WebsocketAPI::curl_ws_sessions.emplace(new_session_id, new_curl_session);
    WebsocketAPI::curl_multi_handles.emplace(new_session_id, new_curl_multi_handle);
    WebsocketAPI::id_sessions.emplace(
        new_session_id, std::make_shared<ws_session_id_t>(new_session_id));
    /* Mark as running until curl_loop explicitly transitions to stopped. */
    WebsocketAPI::curl_ws_session_still_running.emplace(
        new_session_id, std::make_shared<int>(1));
    WebsocketAPI::session_RX_frames_deque.emplace(
        new_session_id,
        std::make_shared<std::deque<ws_incomming_data_t>>());
    WebsocketAPI::session_TX_frames_deque.emplace(
        new_session_id,
        std::make_shared<std::deque<ws_outgoing_data_t>>());
    WebsocketAPI::session_RX_buffer.emplace(new_session_id,
                                            std::make_shared<std::string>(""));
    WebsocketAPI::session_shutdown_requested.emplace(
        new_session_id, std::make_shared<bool>(false));
    WebsocketAPI::session_handshake_ready.emplace(
        new_session_id, std::make_shared<bool>(false));
    WebsocketAPI::session_mutex.emplace(new_session_id,
                                        std::make_shared<std::mutex>());
    WebsocketAPI::session_triggers.emplace(
        new_session_id, std::make_shared<std::condition_variable>());
    WebsocketAPI::session_RX_thread.emplace(new_session_id,
                                            std::make_shared<std::thread>());
    WebsocketAPI::session_TX_thread.emplace(new_session_id,
                                            std::make_shared<std::thread>());
  }

  /* Launch the flush TX messages (outgoing messages) thread (joined on finalize). */
  {
    LOCK_GUARD(WebsocketAPI::global_ws_mutex);
    WebsocketAPI::session_TX_thread.at(new_session_id) =
        std::make_shared<std::thread>(WebsocketAPI::flush_messages_loop,
                                      new_session_id);
  }

  /* Log */
  log_dbg("[success] New Websocket session created with session_id[ %d ].\n", new_session_id);

  return new_session_id;
}

/*
  Wait to flush
    - waits until session_id TX deque has been flushed
*/
void WebsocketAPI::ws_wait_to_flush(const ws_session_id_t session_id) {
  auto session_mtx = WebsocketAPI::get_session_mutex(session_id);
  auto tx_deque = WebsocketAPI::get_session_TX_deque(session_id);
  auto trigger = WebsocketAPI::get_session_trigger(session_id);
  std::unique_lock<std::mutex> lock(*session_mtx);
  const bool flushed = trigger->wait_for(lock, WS_MAX_WAIT, [tx_deque] {
    return tx_deque->empty();
  });
  if (!flushed) {
    log_warn("Timeout while waiting TX flush on session_id[ %d ]\n", session_id);
  }
}

/*
  Wait for curl loop to finish
    - waits until the curl loop has finished
*/
void WebsocketAPI::ws_wait_loop_to_finish(const ws_session_id_t session_id) {
  auto session_mtx = WebsocketAPI::get_session_mutex(session_id);
  auto still_running = WebsocketAPI::get_session_still_running(session_id);
  auto trigger = WebsocketAPI::get_session_trigger(session_id);
  std::unique_lock<std::mutex> lock(*session_mtx);
  const bool stopped = trigger->wait_for(lock, WS_MAX_WAIT, [still_running] {
    return *still_running == 0;
  });
  if (!stopped) {
    log_warn("Timeout while waiting curl loop stop on session_id[ %d ]\n", session_id);
  }
}

/*
  Wait for a response frame to match a frame_id
    - waits until the curl loop has finished
*/
bool WebsocketAPI::ws_wait_server_response(const ws_session_id_t session_id, const std::string target_frame_id) {
  auto session_mtx = WebsocketAPI::get_session_mutex(session_id);
  auto rx_deque = WebsocketAPI::get_session_RX_deque(session_id);
  auto trigger = WebsocketAPI::get_session_trigger(session_id);
  std::unique_lock<std::mutex> lock(*session_mtx);
  bool condition_met = trigger->wait_for(lock, WS_MAX_WAIT, [rx_deque, &target_frame_id] {
    const auto& deque = *rx_deque;
    for (auto it = deque.rbegin(); it != deque.rend(); ++it) {
      if (it->frame_id == target_frame_id) { return true; }
    }
    return false;
  });

  /* Timeout */
  if (!condition_met) {
    log_warn("Timeout condition reached while awaiting server response on session_id[ %d ] waiting for frame_id[ %s ]\n",
      session_id, target_frame_id.c_str());
  }

  return condition_met;
}

/* 
  Connection method (public)
    - finalize connection
*/
void WebsocketAPI::ws_finalize(const ws_session_id_t session_id) {
  log_dbg("%s with session_id[ %d ]...\n", "Finalizing WebSocket connection", session_id);
  /* Validate */
  if (session_id == NULL_CURL_SESSION) {
    log_warn("Unable to finalize NULL session_id[ %d ]; continuing as expected...\n", session_id);
    return;
  }

  auto session_mtx = WebsocketAPI::get_session_mutex(session_id);
  auto trigger = WebsocketAPI::get_session_trigger(session_id);
  auto shutdown_requested = WebsocketAPI::get_session_shutdown_flag(session_id);
  auto tx_thread = WebsocketAPI::get_session_TX_thread(session_id);
  auto rx_thread = WebsocketAPI::get_session_RX_thread(session_id);
  CURL* curl_session = WebsocketAPI::get_session(session_id);
  CURLM* multi_handle = WebsocketAPI::get_session_multi_handle(session_id);
  auto tx_deque = WebsocketAPI::get_session_TX_deque(session_id);

  /* Atomically enqueue close frame and block further writes. */
  std::string close_frame_id = "close-skipped";
  {
    LOCK_GUARD(*session_mtx);
    if (!*shutdown_requested) {
      ws_outgoing_data_t close_frame{};
      const unsigned short close_code = ws_htons(WS_NORMAL_TERMINATION);
      close_frame.frame_data.resize(sizeof(close_code));
      memcpy(close_frame.frame_data.data(), &close_code, sizeof(close_code));
      close_frame.frame_size = close_frame.frame_data.size();
      close_frame.frame_type = CURLWS_CLOSE;
      close_frame.frame_id =
          cuwacunu::piaabo::generate_random_string(CLOSE_FRAME_ID_FORMAT);
      close_frame.local_timestamp = std::chrono::system_clock::now();
      close_frame_id = close_frame.frame_id;
      tx_deque->push_back(std::move(close_frame));
    }
    *shutdown_requested = true;
  }
  trigger->notify_all();

  /* Wait until the TX deque is flushed */
  WebsocketAPI::ws_wait_to_flush(session_id);

  /* Wait until the curl_loop is done */
  WebsocketAPI::ws_wait_loop_to_finish(session_id);

  if (tx_thread->joinable() &&
      tx_thread->get_id() != std::this_thread::get_id()) {
    tx_thread->join();
  }
  if (rx_thread->joinable() &&
      rx_thread->get_id() != std::this_thread::get_id()) {
    rx_thread->join();
  }

  /* Terminate curl handles */
  {
    LOCK_GUARD(*session_mtx);
    curl_multi_remove_handle(multi_handle, curl_session);
    curl_multi_cleanup(multi_handle);
    curl_easy_cleanup(curl_session);
  } /* Unlock */

  /* Finalize session maps */
  WebsocketAPI::remove_session(session_id);

  log_dbg("Finalized WebSocket connection with session_id[ %d ] frame_id[ %s ].\n", 
    session_id, close_frame_id.c_str());
}

/* 
  Connection method (public)
    - initialize connection
*/
ws_session_id_t WebsocketAPI::ws_init(const std::string& url) {
  /* Create curl session */
  ws_session_id_t session_id = WebsocketAPI::initialize_curl_ws_session();
  if (session_id == NULL_CURL_SESSION) { return NULL_CURL_SESSION; }

  /* Get curl session from the session_id */
  CURL* curl_session = WebsocketAPI::get_session(session_id);
  CURLM* multi_handle = WebsocketAPI::get_session_multi_handle(session_id);
  auto session_id_ref = WebsocketAPI::get_session_id_ref(session_id);
  auto session_mtx = WebsocketAPI::get_session_mutex(session_id);
  auto trigger = WebsocketAPI::get_session_trigger(session_id);
  auto handshake_ready = WebsocketAPI::get_session_handshake_flag(session_id);
  auto shutdown_requested = WebsocketAPI::get_session_shutdown_flag(session_id);
  auto still_running = WebsocketAPI::get_session_still_running(session_id);
  
  /* Configure curl session for websockets */
  curl_easy_setopt(curl_session, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl_session, CURLOPT_WRITEFUNCTION, WebsocketAPI::websocket_RX_callback);
  curl_easy_setopt(curl_session, CURLOPT_WRITEDATA, session_id_ref.get());
  curl_easy_setopt(curl_session, CURLOPT_SSL_VERIFYPEER, 1L);
  curl_easy_setopt(curl_session, CURLOPT_SSL_VERIFYHOST, 2L);
  curl_easy_setopt(curl_session, CURLOPT_CONNECTTIMEOUT, 5L);
  curl_easy_setopt(curl_session, CURLOPT_SERVER_RESPONSE_TIMEOUT, 10L);
  curl_easy_setopt(curl_session, CURLOPT_BUFFERSIZE, CURL_MAX_WRITE_SIZE);

  curl_easy_setopt(curl_session, CURLOPT_VERBOSE, 0L);
  

  /* // Optional setup options, to be reviewed
    curl_easy_setopt(curl_session, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl_session, CURLOPT_MAXREDIRS, 5L);
    curl_easy_setopt(curl_session, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl_session, CURLOPT_SOCKOPTFUNCTION , WebsocketAPI::websocket_RX_callback);
    curl_easy_setopt(curl_session, CURLOPT_SOCKOPTDATA, &(WebsocketAPI::id_sessions.at(session_id)));
    curl_easy_setopt(curl_session, CURLOPT_CONNECT_ONLY, 2L);
    char errbuf[CURL_ERROR_SIZE];
    curl_easy_setopt(curl_session, CURLOPT_ERRORBUFFER, errbuf);

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Custom-Header: Value");
    curl_easy_setopt(curl_session, CURLOPT_HTTPHEADER, headers);
  */

  /* Add the session to the curl-multi-object */
  const CURLMcode add_rc = curl_multi_add_handle(
      multi_handle, curl_session); /* Only add one session per multi_handle */
  if (add_rc != CURLM_OK) {
    {
      LOCK_GUARD(*session_mtx);
      *shutdown_requested = true;
      *still_running = 0;
    }
    trigger->notify_all();
    auto tx_thread = WebsocketAPI::get_session_TX_thread(session_id);
    if (tx_thread->joinable() &&
        tx_thread->get_id() != std::this_thread::get_id()) {
      tx_thread->join();
    }
    curl_multi_cleanup(multi_handle);
    curl_easy_cleanup(curl_session);
    WebsocketAPI::remove_session(session_id);
    log_err("Failed to add websocket handle to multi stack for session_id[ %d ]: %s\n",
            session_id, curl_multi_strerror(add_rc));
    return NULL_CURL_SESSION;
  }

  /* Launch a thread for the curl loop to run on this session */
  {
    LOCK_GUARD(WebsocketAPI::global_ws_mutex);
    WebsocketAPI::session_RX_thread.at(session_id) =
        std::make_shared<std::thread>(WebsocketAPI::curl_loop, session_id);
  }

  /* Wait for a successful scheme change or early failure. */
  std::unique_lock<std::mutex> lock(*session_mtx);
  const bool signaled = trigger->wait_for(lock, WS_MAX_WAIT, [handshake_ready, still_running]() {
    return *handshake_ready || *still_running == 0;
  });
  const bool handshake_ok = signaled && *handshake_ready;
  lock.unlock();

  if (!handshake_ok) {
    {
      LOCK_GUARD(*session_mtx);
      *shutdown_requested = true;
    }
    trigger->notify_all();
    log_err("WebSocket handshake failed or timed out for session_id[ %d ]\n", session_id);
    WebsocketAPI::ws_finalize(session_id);
    return NULL_CURL_SESSION;
  }

  log_dbg("[success] WebSocket connection established, session_id[ %d ]\n", session_id);
  
  return session_id;
}

/* 
  Send a ping frame:
    - push a message to the TX deque
*/
std::string WebsocketAPI::ws_write_ping(const ws_session_id_t session_id, const std::string frame_id) {
  auto session_mtx = WebsocketAPI::get_session_mutex(session_id);
  auto tx_deque = WebsocketAPI::get_session_TX_deque(session_id);
  auto shutdown_requested = WebsocketAPI::get_session_shutdown_flag(session_id);
  auto trigger = WebsocketAPI::get_session_trigger(session_id);

  /* Create the Ping frame object */
  ws_outgoing_data_t frame_to_deque;
  std::string return_frame_id;
  {
    /* Fill the frame data with the Ping payload */
    frame_to_deque.frame_data = {};
    frame_to_deque.frame_size = 0;
    frame_to_deque.frame_type = CURLWS_PING;
    frame_to_deque.frame_id = frame_id != "" ? frame_id : cuwacunu::piaabo::generate_random_string(FRAME_ID_FORMAT);
    frame_to_deque.local_timestamp = std::chrono::system_clock::now();
  }
  /* Push to deque */
  {
    LOCK_GUARD(*session_mtx);
    if (*shutdown_requested) {
      log_warn("Ignoring ws_write_ping on finalizing session_id[ %d ]\n",
               session_id);
      return "";
    }
    return_frame_id = frame_to_deque.frame_id;
    tx_deque->push_back(std::move(frame_to_deque));
  }
  trigger->notify_all();
  return return_frame_id;
}

/* 
  Send a pong frame:
    - push a message to the TX deque
*/
std::string WebsocketAPI::ws_write_pong(const ws_session_id_t session_id, const std::string frame_id) {
  auto session_mtx = WebsocketAPI::get_session_mutex(session_id);
  auto tx_deque = WebsocketAPI::get_session_TX_deque(session_id);
  auto shutdown_requested = WebsocketAPI::get_session_shutdown_flag(session_id);
  auto trigger = WebsocketAPI::get_session_trigger(session_id);

  /* Create the Pong frame object */
  ws_outgoing_data_t frame_to_deque;
  std::string return_frame_id;
  {
    /* Fill the frame data with the Pong payload */
    frame_to_deque.frame_data = {};
    frame_to_deque.frame_size = 0;
    frame_to_deque.frame_type = CURLWS_PONG;
    frame_to_deque.frame_id = frame_id != "" ? frame_id : cuwacunu::piaabo::generate_random_string(FRAME_ID_FORMAT);
    frame_to_deque.local_timestamp = std::chrono::system_clock::now();
  }
  /* Push to deque */
  {
    LOCK_GUARD(*session_mtx);
    if (*shutdown_requested) {
      log_warn("Ignoring ws_write_pong on finalizing session_id[ %d ]\n",
               session_id);
      return "";
    }
    return_frame_id = frame_to_deque.frame_id;
    tx_deque->push_back(std::move(frame_to_deque));
  }
  trigger->notify_all();
  return return_frame_id;
}

/* 
  Send a close frame:
    - push a message to the TX deque
*/
std::string WebsocketAPI::ws_write_close(const ws_session_id_t session_id, unsigned short closing_code, const std::string frame_id) {
  auto session_mtx = WebsocketAPI::get_session_mutex(session_id);
  auto tx_deque = WebsocketAPI::get_session_TX_deque(session_id);
  auto shutdown_requested = WebsocketAPI::get_session_shutdown_flag(session_id);
  auto trigger = WebsocketAPI::get_session_trigger(session_id);

  /* Create the object */
  ws_outgoing_data_t frame_to_deque;
  std::string return_frame_id;
  {
    unsigned short close_code = ws_htons(closing_code); /* Network byte order */
    /* Fill the frame object */
    frame_to_deque.frame_data.resize(sizeof(close_code));
    memcpy(frame_to_deque.frame_data.data(), &close_code, sizeof(close_code));
    frame_to_deque.frame_size = frame_to_deque.frame_data.size();
    frame_to_deque.frame_type = CURLWS_CLOSE;
    frame_to_deque.frame_id = frame_id != "" ? frame_id : cuwacunu::piaabo::generate_random_string(CLOSE_FRAME_ID_FORMAT);
    frame_to_deque.local_timestamp = std::chrono::system_clock::now();
  }
  /* Push to deque */
  {
    LOCK_GUARD(*session_mtx);
    if (*shutdown_requested) {
      log_warn("Ignoring ws_write_close on finalizing session_id[ %d ]\n",
               session_id);
      return "";
    }
    return_frame_id = frame_to_deque.frame_id;
    tx_deque->push_back(std::move(frame_to_deque));
  }
  trigger->notify_all();
  return return_frame_id;
}

/* 
  Send a binary frame:
    - push a message to the TX deque
*/
std::string WebsocketAPI::ws_write_binary(const ws_session_id_t session_id, const std::vector<unsigned char>& data, const std::string frame_id) {
  auto session_mtx = WebsocketAPI::get_session_mutex(session_id);
  auto tx_deque = WebsocketAPI::get_session_TX_deque(session_id);
  auto shutdown_requested = WebsocketAPI::get_session_shutdown_flag(session_id);
  auto trigger = WebsocketAPI::get_session_trigger(session_id);

  /* Create the object */
  ws_outgoing_data_t frame_to_deque;
  std::string return_frame_id;
  {
    /* Fill the frame object */
    frame_to_deque.frame_size = data.size();
    frame_to_deque.frame_data = data;
    frame_to_deque.frame_type = CURLWS_BINARY;
    frame_to_deque.frame_id = frame_id != "" ? frame_id : cuwacunu::piaabo::generate_random_string(FRAME_ID_FORMAT);
    frame_to_deque.local_timestamp = std::chrono::system_clock::now();
  }
  /* Push to deque */
  {
    LOCK_GUARD(*session_mtx);
    if (*shutdown_requested) {
      log_warn("Ignoring ws_write_binary on finalizing session_id[ %d ]\n",
               session_id);
      return "";
    }
    return_frame_id = frame_to_deque.frame_id;
    tx_deque->push_back(std::move(frame_to_deque));
  }
  trigger->notify_all();
  return return_frame_id;
}

/* 
  Send a string frame:
    - push a message to the TX deque
*/
std::string WebsocketAPI::ws_write_text(const ws_session_id_t session_id, std::string data, const std::string frame_id) {
  auto session_mtx = WebsocketAPI::get_session_mutex(session_id);
  auto tx_deque = WebsocketAPI::get_session_TX_deque(session_id);
  auto shutdown_requested = WebsocketAPI::get_session_shutdown_flag(session_id);
  auto trigger = WebsocketAPI::get_session_trigger(session_id);

  /* Create the object */
  ws_outgoing_data_t frame_to_deque;
  std::string return_frame_id;
  {
    std::vector<unsigned char> data_vect(data.begin(), data.end()); /* ASCII encoding assumption */
    /* Fill the frame object */
    frame_to_deque.frame_data = std::move(data_vect);
    frame_to_deque.frame_id   = frame_id != "" ? frame_id : cuwacunu::piaabo::generate_random_string(FRAME_ID_FORMAT);
    frame_to_deque.frame_type = CURLWS_TEXT;
    frame_to_deque.frame_size = data.size();
    frame_to_deque.local_timestamp = std::chrono::system_clock::now();
  }
  /* Push to deque */
  {
    LOCK_GUARD(*session_mtx);
    if (*shutdown_requested) {
      log_warn("Ignoring ws_write_text on finalizing session_id[ %d ]\n",
               session_id);
      return "";
    }
    return_frame_id = frame_to_deque.frame_id;
    tx_deque->push_back(std::move(frame_to_deque));
  }
  trigger->notify_all();
  return return_frame_id;
}

/*
  Await and retrieve server response
*/
std::optional<ws_incomming_data_t> WebsocketAPI::ws_await_and_retrive_server_response(const ws_session_id_t session_id, const std::string target_frame_id) {
  auto session_mtx = WebsocketAPI::get_session_mutex(session_id);
  auto rx_deque = WebsocketAPI::get_session_RX_deque(session_id);
  
  /* Wait for the server to respond */
  bool condition_met = cuwacunu::piaabo::curl::WebsocketAPI::ws_wait_server_response(session_id, target_frame_id);
  
  /* Failure: if no response was received (await timeout) */
  if (!condition_met) {
    return std::nullopt;
  }

  /* Success: response from the server was retrieved */
  { 
    /* Lock */
    LOCK_GUARD(*session_mtx);
    
    /* Retrieve the response (and remove it from the queue) */
    auto &deque = *rx_deque;
    for (std::size_t idx = deque.size(); idx > 0; --idx) {
      std::size_t i = idx - 1; /* Adjust index for counting down */
      if (deque[i].frame_id == target_frame_id) {
        /* Store the element before erasing */
        auto result = deque[i];
        /* Erase the element from the deque */
        deque.erase(deque.begin() + i);
        /* Return the saved element */
        return result;
      }
    }
  } /* Unlock */

  /* Failure: unexpected disappearance */
  log_err("Unexpected disappearance while retrieving deque element frame_id[ %s ] at session_id[ %d ]\n", 
    target_frame_id.c_str(), session_id);
  return std::nullopt;
}

/* 
  Main curl_multi loop 
    used to process the activity of the curl handles (sessions)
*/
void WebsocketAPI::curl_loop(const ws_session_id_t session_id) {
  log_dbg("Dispatching a new curl-thread on session_id[ %d ].\n", session_id);
  CURL* curl_session = WebsocketAPI::get_session(session_id);
  CURLM* multi_handle = WebsocketAPI::get_session_multi_handle(session_id);
  auto session_mtx = WebsocketAPI::get_session_mutex(session_id);
  auto trigger = WebsocketAPI::get_session_trigger(session_id);
  auto still_running_state = WebsocketAPI::get_session_still_running(session_id);
  auto shutdown_requested = WebsocketAPI::get_session_shutdown_flag(session_id);
  auto handshake_ready = WebsocketAPI::get_session_handshake_flag(session_id);

  bool notified_scheme_change = false;
  int numfds = 0;
  int still_running_local = 0;
  CURLMcode res_code = CURLM_OK;

  while (true) {
    {
      LOCK_GUARD(*session_mtx);
      if (*shutdown_requested) break;
    }

    res_code = curl_multi_perform(multi_handle, &still_running_local);
    {
      LOCK_GUARD(*session_mtx);
      *still_running_state = still_running_local;
    }
    trigger->notify_all();
    if (res_code != CURLM_OK) {
      log_err("Failed to perform curl_multi operation with error: %s\n",
              curl_multi_strerror(res_code));
      break;
    }

    {
      LOCK_GUARD(*session_mtx);
      if (*shutdown_requested) break;
    }

    res_code = curl_multi_wait(multi_handle, NULL, 0, 1000, &numfds);
    if (res_code != CURLM_OK) {
      log_err("curl_multi_wait() failed: %s\n", curl_multi_strerror(res_code));
      break;
    }

    {
      LOCK_GUARD(*session_mtx);
      if (*shutdown_requested) break;
    }

    /* Verify connection failure, overall verify errors */
    CURLMsg* curl_dbg_msg = nullptr;
    int dbg_msgs_left = 0;
    while ((curl_dbg_msg = curl_multi_info_read(multi_handle, &dbg_msgs_left))) {
      if (curl_dbg_msg->msg == CURLMSG_DONE) {
        if (curl_dbg_msg->data.result == CURLE_COULDNT_RESOLVE_HOST) {
          log_err("Curl failed to resolve host (no internet). %s\n",
                  curl_easy_strerror(curl_dbg_msg->data.result));
        } else if (curl_dbg_msg->data.result == CURLE_COULDNT_CONNECT) {
          log_err("Curl failed to connect or shutting down connection. %s\n",
                  curl_easy_strerror(curl_dbg_msg->data.result));
        } else if (curl_dbg_msg->data.result != CURLE_OK) {
          log_err("Curl general error: %s\n",
                  curl_easy_strerror(curl_dbg_msg->data.result));
        }
      }
    }

    /* Verify scheme change */
    if (!notified_scheme_change) {
      char* scheme = NULL;
      if (curl_easy_getinfo(curl_session, CURLINFO_SCHEME, &scheme) == CURLE_OK) {
        if (scheme && (strcmp(scheme, "WS") == 0 || strcmp(scheme, "WSS") == 0)) {
          long response_code = 0;
          const CURLcode res =
              curl_easy_getinfo(curl_session, CURLINFO_RESPONSE_CODE, &response_code);
          if (res == CURLE_OK && response_code == 101) {
            notified_scheme_change = true;
            {
              LOCK_GUARD(*session_mtx);
              *handshake_ready = true;
            }
            trigger->notify_all();
          }
        }
      }
    }

    if (still_running_local <= 0) break;
  }

  /* Curl ran out of jobs */
  CLEAR_SYS_ERR(); /* Curl triggers some errors that are not critical */
  log_dbg("[success] curl-thread session_id[ %d ] finished operating.\n", session_id);

  {
    LOCK_GUARD(*session_mtx);
    *still_running_state = 0;
  }
  trigger->notify_all();
  
  return;
}

/*
  Flush (TX) messages
    - runs an independent thread for every session_id
    - trigger (notify) when on WebsocketAPI::ws_write_* methods
*/
void WebsocketAPI::flush_messages_loop(const ws_session_id_t session_id) {
  auto session_mtx = WebsocketAPI::get_session_mutex(session_id);
  auto tx_deque = WebsocketAPI::get_session_TX_deque(session_id);
  auto shutdown_requested = WebsocketAPI::get_session_shutdown_flag(session_id);
  auto trigger = WebsocketAPI::get_session_trigger(session_id);
  CURL* curl_session = WebsocketAPI::get_session(session_id);

  while (true) {
    ws_outgoing_data_t frame_from_deque;
    bool should_send = false;
    {
      std::unique_lock<std::mutex> lock(*session_mtx);
      trigger->wait_for(lock, WS_MAX_WAIT, [shutdown_requested, tx_deque]() {
        return *shutdown_requested || !tx_deque->empty();
      });
      if (*shutdown_requested && tx_deque->empty()) {
        log_dbg("Stop flushing messages for session_id[ %d ]\n", session_id);
        break;
      }
      if (!tx_deque->empty()) {
        frame_from_deque = std::move(tx_deque->front());
        tx_deque->pop_front();
        should_send = true;
      }
    }

    if (!should_send) continue;

    /* Send the message outside lock. */
    const CURLcode res = send_ws_frame(
        curl_session, frame_from_deque.frame_data.data(),
        frame_from_deque.frame_size, frame_from_deque.frame_type);

    /* Validate the response */
    if (res != CURLE_OK) {
      log_err("Unable to send frame_id[%s] from session_id[ %d ], with error: %s\n",
              frame_from_deque.frame_id.c_str(), session_id, curl_easy_strerror(res));
    } else {
      log_secure_dbg("[success] Sent session_id[ %d ]'s message with frame_id[ %s ]\n",
                     session_id, frame_from_deque.frame_id.c_str());
    }

    trigger->notify_all();
  }

  trigger->notify_all();
}

/* 
  Write_callback method, 
    - on receiving a message it will add the data to the session deque. 
    - there is a separate thread that processes those deques; the writeback is left simple without the logic of interpreting the data.
*/
size_t WebsocketAPI::websocket_RX_callback(char* ptr, size_t size, size_t nmemb, void* userdata) {
  /* Get the message arrival time */
  std::chrono::system_clock::time_point local_timestamp = std::chrono::system_clock::now();

  if (userdata == nullptr || ptr == nullptr) return size * nmemb;

  /* Interpret the session_id */
  ws_session_id_t session_id = *(ws_session_id_t*)userdata;
  auto session_mtx = WebsocketAPI::get_session_mutex(session_id);
  auto rx_buffer = WebsocketAPI::get_session_RX_buffer(session_id);
  auto rx_deque = WebsocketAPI::get_session_RX_deque(session_id);
  auto trigger = WebsocketAPI::get_session_trigger(session_id);
  ws_incomming_data_t frame_to_deque;
  bool produced_complete_frame = false;
    
  /* Lock the current session mutex */
  {
    LOCK_GUARD(*session_mtx);

    /* Push the received RX frame to the session_id FIFO deque */
    {
      /* Append the incoming buffer */
      *rx_buffer += std::string(ptr, size * nmemb);
      if (rx_buffer->size() > kWsMaxRxBufferBytes) {
        log_warn("Websocket session_id[ %d ] RX buffer exceeded %ld bytes; dropping partial frame.\n",
                 session_id, static_cast<long>(kWsMaxRxBufferBytes));
        rx_buffer->clear();
      }

      /* Validate if the callback was invoked on a chunk or the total */
      if (!rx_buffer->empty() &&
          cuwacunu::piaabo::json_fast_validity_check(*rx_buffer)) {
        /* Total data was reached in the chunk */
        frame_to_deque.data = *rx_buffer;
        frame_to_deque.local_timestamp = local_timestamp;
        frame_to_deque.frame_id = cuwacunu::piaabo::extract_json_string_value(frame_to_deque.data, "id", "NULL");
        
        /* Reset buffer */
        rx_buffer->clear();
        produced_complete_frame = true;

        /* Log info */
        log_secure_info("[total] Websocket session_id[ %d ] callback received frame_id[ %s ]\n", 
          session_id, 
          frame_to_deque.frame_id.c_str());
        log_secure_dbg("[total] Websocket session_id[ %d ] callback received frame_id[ %s ] bytes=%ld\n", 
          session_id, 
          frame_to_deque.frame_id.c_str(), 
          static_cast<long>(frame_to_deque.data.size()));
      } else {
        log_secure_dbg("[chunk] Websocket session_id[ %d ] callback received data chunk of size: %ld\n", 
          session_id, 
          size * nmemb);
      }
    }
    if (produced_complete_frame) {
      rx_deque->push_back(std::move(frame_to_deque));
    }
  } /* Unlock */

  if (produced_complete_frame) trigger->notify_all();

  /* Return the number of processed bytes; in this case, we return the total count */
  return size * nmemb;
}

} /* namespace curl */
} /* namespace piaabo */
} /* namespace cuwacunu */
