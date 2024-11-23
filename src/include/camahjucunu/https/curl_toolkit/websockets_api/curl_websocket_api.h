/* curl_websocket_api.h */
#pragma once

/* CURL implementing RFC 6455, we just use CURL */

#include "piaabo/dutils.h"
#include "piaabo/json_parsing.h"
#include "piaabo/darchitecture.h"
#include "camahjucunu/https/curl_toolkit/curl_utils.h"
#include <thread>
#include <chrono>
#include <mutex>
#include <deque>
#include <unordered_map>
#include <condition_variable>

RUNTIME_WARNING("(curl_websocket_api.h)[] fatal error on unknown session_id (fatal might be a good thing here there shodun't be a reason to allow undefined instructions).\n");
RUNTIME_WARNING("(curl_websocket_api.h)[ws_incomming_data_t] not necesarly local_timestamp matches the timestamps in the body of the responces. \n");
RUNTIME_WARNING("(curl_websocket_api.h)[ws_incomming_data_t] this implementation (on deques) relies on the ability of the server to retrive an \"id\" key on the data, to track which incommind frame correspond to which output frame. This can be changed for other types of WS interactions, for now, this implementation is tailored to interacti with binance or alike servers. \n");
RUNTIME_WARNING("(curl_websocket_api.h)[] writing to dbg might be slow if dbg is checking config every time.\n");
RUNTIME_WARNING("(curl_websocket_api.h)[] TX_deque for orders, might delay the sending of instructions, so include time_window in the instruction.\n");
RUNTIME_WARNING("(curl_websocket_api.h)[] many curl options are uncommented, this needs to be reviewed.\n");
RUNTIME_WARNING("(curl_websocket_api.h)[ws_write] add encoding support for ws_write_text.\n");
RUNTIME_WARNING("(curl_websocket_api.h)[] log the times and redirection count (curl_easy_getinfo()).\n");
RUNTIME_WARNING("(curl_websocket_api.h)[] split header into implementation file .cpp (maybe not).\n");
RUNTIME_WARNING("(curl_websocket_api.h)[] fix the possible infinite waits.\n");
RUNTIME_WARNING("(curl_websocket_api.h)[] CURLOPT_BUFFERSIZE has a maximun, breaking large data responses in chunks on websocket_RX_callback, a server might mix these responses, making it impossible to retrive the complete message (binance seems to respect this alright).\n");
RUNTIME_WARNING("(curl_websocket_api.h)[] websocket_RX_callback expects data to be a valid json format (complete or separated in multiple chunks).\n");


#define WS_NORMAL_TERMINATION                 (uint16_t) 0x3E8  /* Normal closure; the connection successfully completed whatever purpose for which it was created. */
#define WS_GOING_AWAY_TERMINATION             (uint16_t) 0x3E9  /* Going away; the server is going down, or the browser is navigating away from the page that opened the connection. */
#define WS_PROTOCOL_ERROR_TERMINATION         (uint16_t) 0x3EA  /* Protocol error; the endpoint is terminating the connection due to a protocol error. */
#define WS_UNSUPORTED_DATA_TERMINATION        (uint16_t) 0x3EB  /* Unsupported data; the endpoint received data of a type it cannot accept (for example, a text-only endpoint receiving binary data). */
#define WS_INVALID_FRAME_TERMINATION          (uint16_t) 0x3EF  /* Invalid frame payload data; the endpoint received data inconsistent with the type of the message (e.g., non-UTF-8 data within a text message). */
#define WS_INTERNAL_SERVER_ERROR_TERMINATION  (uint16_t) 0x3F3  /* Internal server error; the server is terminating the connection due to a problem it encountered. */

#define FRAME_ID_FORMAT "xxxx-xxxx-xxxx"
#define CLOSE_FRAME_ID_FORMAT "close-xxxx-xxxx"

#define WS_MAX_WAIT std::chrono::seconds(5)

namespace cuwacunu {
namespace camahjucunu {
namespace curl {

using ws_session_id_t = int;
using ws_WriteCallback_fn = size_t (*)(char*, size_t, size_t, void*);

struct ws_incomming_data_t {
  std::string frame_id;
  std::string data;
  std::chrono::system_clock::time_point local_timestamp; /* timestamp at the moment of receiving the last chunk of frame (or ideally total frame) */
};
ENFORCE_ARCHITECTURE_DESIGN(ws_incomming_data_t);

struct ws_outgoing_data_t {
  std::string frame_id;
  int     frame_type;
  size_t  frame_size;
  std::vector<unsigned char> frame_data;
  std::chrono::system_clock::time_point local_timestamp;
};
ENFORCE_ARCHITECTURE_DESIGN(ws_outgoing_data_t);

struct WebsocketAPI {
private:
  /* 
    Singleton (static) variables
      - curl_ws_session_still_running, number of running process per curl_session
      - map to a curl sessions, one per session
      - map to a curl_multi handlers, one per session
      - map to a mutex, one per session
      - map to a session id, one per session
      - map to a deque of received data, one per session
      - map to a deque of outgoing data, one per session
      - session RX message buffer to join chunks of input messages into a whole
      - global (in the context of WebsocketAPI) mutex
      - sessions_counter; to create unique ids for every session
      - session thread for the RX (incoming) events
      - session thread for the TX (outgoing) events
      - session conditional trigger variable
  */
  static std::unordered_map<ws_session_id_t, int> curl_ws_session_still_running;
  static std::unordered_map<ws_session_id_t, CURL*> curl_ws_sessions;
  static std::unordered_map<ws_session_id_t, CURLM *> curl_multi_handles;
  static std::unordered_map<ws_session_id_t, ws_session_id_t> id_sessions;  /* this unusual map is to preserve in memory a reference to the ws_session_id when passed to the curl constructor */
  static std::unordered_map<ws_session_id_t, std::deque<ws_incomming_data_t>> session_RX_frames_deque;
  static std::unordered_map<ws_session_id_t, std::deque<ws_outgoing_data_t>> session_TX_frames_deque;
  static std::unordered_map<ws_session_id_t, std::string> session_RX_buffer;
  static std::unordered_map<ws_session_id_t, std::mutex> session_mutex;
  static std::unordered_map<ws_session_id_t, std::thread> session_RX_thread;
  static std::unordered_map<ws_session_id_t, std::thread> session_TX_thread;
  static std::unordered_map<ws_session_id_t, std::condition_variable> session_triggers;
  static std::mutex global_ws_mutex;
  static int sessions_counter;

  /*
    Enforce Singleton requirements
  */
  WebsocketAPI();
  ~WebsocketAPI();

  WebsocketAPI(const WebsocketAPI&) = delete;
  WebsocketAPI& operator=(const WebsocketAPI&) = delete;
  WebsocketAPI(WebsocketAPI&&) = delete;
  WebsocketAPI& operator=(WebsocketAPI&&) = delete;

public:
  /*
    Access point Constructor
  */
  static struct _init { public: _init(); } _initializer;
  static void init();
  static void finit();

private:
  /* 
    Session utils (private)
      - get session
      - get session mutex
      - remove session from the heap
  */
  static CURL* get_session(const ws_session_id_t session_id);
  static std::mutex* get_session_mutex(const ws_session_id_t session_id);
  static std::deque<ws_incomming_data_t>* get_session_RX_deque(const ws_session_id_t session_id);
  static std::deque<ws_outgoing_data_t>* get_session_TX_deque(const ws_session_id_t session_id);
  static void remove_session(const ws_session_id_t session_id);

  /* 
    Session utils (private)
      - initialize session
      - disconnect session
  */
  static ws_session_id_t initialize_curl_ws_session();

public:
  /*
    Wait to flush
      - waits until session_id TX deque has been flushed
  */
  static void ws_wait_to_flush(const ws_session_id_t session_id);
  /*
    Wait for curl loop to finish
      - waits until the curl loop has finished
  */
  static void ws_wait_loop_to_finish(const ws_session_id_t session_id);
  /*
    Wait for a response frame to match a frame_id
      - waits until the curl loop has finished
  */
  static bool ws_wait_server_response(const ws_session_id_t session_id, const std::string target_frame_id);

public:
  /* 
    Connection method (public)
      - finalize connection
  */
  static void ws_finalize(const ws_session_id_t session_id);
  /* 
    Connection method (public)
      - initialize connection
  */
  static ws_session_id_t ws_init(const std::string& url);

public:
  /* 
    Send a ping frame:
      - push a message to the TX deque
  */
  static std::string ws_write_ping(const ws_session_id_t session_id, const std::string frame_id = "");
  /* 
    Send a pong frame:
      - push a message to the TX deque
  */
  static std::string ws_write_pong(const ws_session_id_t session_id, const std::string frame_id = "");
  /* 
    Send a close frame:
      - push a message to the TX deque
  */
  static std::string ws_write_close(const ws_session_id_t session_id, unsigned short closing_code, const std::string frame_id = "");
  /* 
    Send a binary frame:
      - push a message to the TX deque
  */
  static std::string ws_write_binary(const ws_session_id_t session_id, const std::vector<unsigned char>& data, const std::string frame_id = "");
  /* 
    Send a string frame:
      - push a message to the TX deque
  */
  static std::string ws_write_text(const ws_session_id_t session_id, std::string data, const std::string frame_id = "");

  /*
    Await and retrieve server response
  */
  static std::optional<ws_incomming_data_t> ws_await_and_retrive_server_response(const ws_session_id_t session_id, const std::string target_frame_id);

private:
  /* 
    Main curl_multi loop 
      used to process the activity of the curl handles (sessions)
  */
  static void curl_loop(const ws_session_id_t session_id);

private:
  /*
    Flush (TX) messages
      - runs an independent thread for every session_id
      - trigger (notify) when on WebsocketAPI::ws_write_* methods
  */
  static void flush_messages_loop(const ws_session_id_t session_id);

private:
  /* 
    Write_callback method, 
      - on receiving a message it will add the data to the session deque. 
      - there is a separate thread that processes those deques; the writeback is left simple without the logic of interpreting the data.
  */
  static size_t websocket_RX_callback(char* ptr, size_t size, size_t nmemb, void* userdata);
};
ENFORCE_SINGLETON_DESIGN(WebsocketAPI);

} /* namespace curl */
} /* namespace camahjucunu */
} /* namespace cuwacunu */
