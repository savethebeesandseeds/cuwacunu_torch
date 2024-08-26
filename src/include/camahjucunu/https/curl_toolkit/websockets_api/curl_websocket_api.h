// cleanup_ws_connection
// cleanup_curl
// send_ws_message
// receive_ws_message
// handle_event
//   open
//   close
//   message

// // Function to create and open a WebSocket connection
// function openWebSocket(url):
//   // Initialize the WebSocket object
//   websocket = new WebSocket(url)

//   // Event handler for when the connection is opened
//   websocket.onopen = function():
//     print("Connection established")
//     // Send a message through the WebSocket
//     sendMessage("Hello, server!")

//   // Event handler for receiving messages from the server
//   websocket.onmessage = function(message):
//     print("Received message: " + message.data)
//     // Handle message based on content or type
//     handleMessage(message.data)

//   // Event handler for errors
//   websocket.onerror = function(error):
//     print("Error occurred: " + error.message)

//   // Event handler for when the connection is closed
//   websocket.onclose = function():
//     print("Connection closed")

// // Function to send a message through the WebSocket
// function sendMessage(message):
//   if websocket.readyState == WebSocket.OPEN:
//     websocket.send(message)
//   else:
//     print("WebSocket is not open")

// // Function to handle incoming messages (you can customize this part)
// function handleMessage(data):
//   // Process the message based on your application's logic
//   print("Processing message: " + data)

// // Main execution flow
// openWebSocket(websocket_url)




// ... reading the queue should be update state machine kinda




#pragma once
#include "piaabo/dutils.h"
#include "piaabo/architecture.h"
#include "camahjucunu/https/curl_toolkit/curl_utils.h"
#include <chrono>
#include <mutex>
#include <queue>
#include <unordered_map>


RUNTIME_WARNING("(curl_websocket_api.h)[] fatal error on unknown session_id (fatal might be a good thing here there shodun't be a reason to allow undefined instructions).\n");
RUNTIME_WARNING("(curl_websocket_api.h)[ws_incomming_data] not necesarly local_timestamp matches the timestamps in the body of the responces. \n");
RUNTIME_WARNING("(curl_websocket_api.h)[] writing to dbg might be slow if dbg is checking config every time.\n");
RUNTIME_WARNING("(curl_websocket_api.h)[] output_queue for orders, might delay the sending of instructions, so include time_window in the instruction.\n");
RUNTIME_WARNING("(curl_websocket_api.h)[] many curl options are uncommented, this needs to be reviewed.\n");


#define CLOSE_FRAME {0x88, 0x02, 0xE8, 0x03} // 0x88 = FIN + opcode 0x8, 0x02 = two bytes payload, 0x03E8 = 1000 in network byte order

namespace cuwacunu {
namespace camahjucunu {
namespace curl {

using ws_session_id_t = int;
using ws_WriteCallback_fn = size_t (*)(char*, size_t, size_t, void*);

struct ws_incomming_data {
  std::string data;
  std::chrono::system_clock::time_point local_timestamp;
};
ENFORCE_ARCHITECTURE_DESIGN(ws_incomming_data);

struct ws_outgoing_data {
  std::string data;
  std::chrono::system_clock::time_point local_timestamp;
};
ENFORCE_ARCHITECTURE_DESIGN(ws_outgoing_data);

struct WebsocketAPI {
private:
/* 
  Singleton (static) variables
    - map to a curl sessions, one per connection
    - map to a mutex, one per connection
    - map to a session id, one per connection
    - map to a queue of received data, one per connection
    - global (in the context of WebsocketAPI) mutex
    - sessions_counter; to create unique ids for every session
*/
  static std::unordered_map<ws_session_id_t, CURL*> curl_ws_sessions;
  static std::unordered_map<ws_session_id_t, ws_session_id_t> id_sessions;  /* this unusual queue is to preserve in memory a reference to the ws_session_id when passed to the curl contructor */
  static std::unordered_map<ws_session_id_t, std::queue<ws_incomming_data>> session_input_frames_queue;
  static std::unordered_map<ws_session_id_t, std::queue<ws_outgoing_data>> session_output_frames_queue;
  static std::unordered_map<ws_session_id_t, std::mutex> session_mutex;
  static std::mutex global_ws_mutex;
  static int sessions_counter;

/*
  Enforce Signleton requirements
*/
  WebsocketAPI() = default;
  ~WebsocketAPI() = default;

  WebsocketAPI(const WebsocketAPI&) = delete;
  WebsocketAPI& operator=(const WebsocketAPI&) = delete;
  WebsocketAPI(WebsocketAPI&&) = delete;
  WebsocketAPI& operator=(WebsocketAPI&&) = delete;
public:
/*
  Access point Contructor
*/
  static struct _init {public:_init(){WebsocketAPI::init();}}_initializer;
  static void init() {
    LOCK_GUARD(WebsocketAPI::global_ws_mutex);
    /* make sure curl is globaly initialized */
    global_init();
  }
private:
/* 
  Session utils (private)
    - get session
    - get session mutex
    - remove session form the heap
*/
  static CURL* get_session(ws_session_id_t session_id) {
    /* validate */
    if (session_id == NULL_CURL_SESSION || curl_ws_sessions.find(session_id) == curl_ws_sessions.end()) {
      log_fatal("%s with id: %d\n", "Failed to identify curl websocket session", session_id);
      return nullptr;
    }
    /* retrive session */
    return curl_ws_sessions.at(session_id);
  }
  static std::mutex* get_session_mutex(ws_session_id_t session_id) {
    /* valdiate */
    if (session_id == NULL_CURL_SESSION || WebsocketAPI::session_mutex.find(session_id) == WebsocketAPI::session_mutex.end()) {
      log_fatal("%s with id: %d\n", "Failed to identify websocket session mutex", session_id);
      return nullptr;
    }
    /* retrive session mutex */
    return &(WebsocketAPI::session_mutex.at(session_id));
  }
  static std::queue<ws_incomming_data>* get_session_input_queue(ws_session_id_t session_id) {
    /* valdiate */
    if (session_id == NULL_CURL_SESSION || WebsocketAPI::session_input_frames_queue.find(session_id) == WebsocketAPI::session_input_frames_queue.end()) {
      log_fatal("%s with id: %d\n", "Failed to identify websocket session mutex", session_id);
      return nullptr;
    }
    /* retrive session mutex */
    return &(WebsocketAPI::session_input_frames_queue.at(session_id));
  }
  static std::queue<ws_outgoing_data>* get_session_output_queue(ws_session_id_t session_id) {
    /* valdiate */
    if (session_id == NULL_CURL_SESSION || WebsocketAPI::session_output_frames_queue.find(session_id) == WebsocketAPI::session_output_frames_queue.end()) {
      log_fatal("%s with id: %d\n", "Failed to identify websocket session mutex", session_id);
      return nullptr;
    }
    /* retrive session mutex */
    return &(WebsocketAPI::session_output_frames_queue.at(session_id));
  }
  static void remove_session(ws_session_id_t session_id) {
    /* free memory */
    {
      LOCK_GUARD(*WebsocketAPI::get_session_mutex(session_id));
      WebsocketAPI::curl_ws_sessions.erase(session_id);
      WebsocketAPI::id_sessions.erase(session_id);
      WebsocketAPI::session_input_frames_queue.erase(session_id);
    }
    WebsocketAPI::session_mutex.erase(session_id);
  }
/* 
  Session utils (private)
    - initialize session
    - disconect session
*/
private:
  static ws_session_id_t initialize_curl_ws_session() {
    /* initialize the session */
    CURL* new_curl_session = create_curl_session();

    /* create the session variables */
    std::mutex new_session_mutex;
    ws_session_id_t new_session_id = WebsocketAPI::sessions_counter;
    std::queue<ws_incomming_data> new_session_input_frames_queue;
    WebsocketAPI::sessions_counter ++;

    /* add the session variables to the session queues */
    curl_ws_sessions.emplace(new_session_id, new_curl_session);
    WebsocketAPI::id_sessions.emplace(new_session_id, new_session_id);;
    WebsocketAPI::session_input_frames_queue.emplace(new_session_id, new_session_input_frames_queue);
    WebsocketAPI::session_mutex.emplace(new_session_id, new_session_mutex);
    
    return new_session_id;
  }
  static void disconnect_ws_session(ws_session_id_t session_id) {
    /* Guard the session mutex */
    LOCK_GUARD(*WebsocketAPI::get_session_mutex(session_id));
    
    /* Retrieve the session */
    CURL* curl_session = WebsocketAPI::get_session(session_id);

    /* Send WebSocket protocol disconnect message */
    unsigned char close_frame[] = CLOSE_FRAME;
    CURLcode res = send_ws_frame(curl_session, close_frame, sizeof(close_frame), CURLWS_BINARY);

    if (res != CURLE_OK) {
      log_err("Unable to disconect session_id[%d], with error: %s\n", session_id, curl_easy_strerror(res));
    }
  }
public:
/* 
  Conextion method (public)
    - finalize connection
*/
  static void finalize_ws_connection(ws_session_id_t session_id) {

    /* Retrieve the session */
    CURL* curl_session = WebsocketAPI::get_session(session_id);

    /* terminate session */
    {
      /* lock */
      LOCK_GUARD(*WebsocketAPI::get_session_mutex(session_id));
      
      /* disconect */
      WebsocketAPI::disconnect_ws_session(session_id);

      /* cleanup (free) memory */
      curl_easy_cleanup(curl_session);
    } /* unlock */
    
    /* finalize session */
    WebsocketAPI::remove_session(session_id);

    log_info("%s with session_id[%d].\n", "Finalized WebSocket connection", session_id);
  }
/* 
  Conextion method (public)
    - initialize connection
*/
public:
  static ws_session_id_t initialize_ws_connection(const std::string& url) {
    /* create curl session */
    ws_session_id_t session_id = WebsocketAPI::initialize_curl_ws_session();
    if(session_id == NULL_CURL_SESSION) { return NULL_CURL_SESSION; }

    /* get curl session from the session_id */
    CURL* curl_session = WebsocketAPI::get_session(session_id);
    /* skiping curl_session validation */
    
    /* configure curl session for websockets */
    curl_easy_setopt(curl_session, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_session, CURLOPT_WRITEFUNCTION, WebsocketAPI::websocket_input_callback);
    curl_easy_setopt(curl_session, CURLOPT_WRITEDATA, &(WebsocketAPI::id_sessions.at(session_id)));
    // curl_easy_setopt(curl_session, CURLOPT_SSL_VERIFYPEER, 1L);
    // curl_easy_setopt(curl_session, CURLOPT_SSL_VERIFYHOST, 2L);
    // curl_easy_setopt(curl_session, CURLOPT_CONNECTTIMEOUT, 10L);
    // curl_easy_setopt(curl_session, CURLOPT_TIMEOUT, 20L);
    // curl_easy_setopt(curl_session, CURLOPT_FOLLOWLOCATION, 1L);
    // curl_easy_setopt(curl_session, CURLOPT_MAXREDIRS, 5L);
    // curl_easy_setopt(curl_session, CURLOPT_VERBOSE, 1L);
    // char errbuf[CURL_ERROR_SIZE];
    // curl_easy_setopt(curl_session, CURLOPT_ERRORBUFFER, errbuf);

    // struct curl_slist* headers = NULL;
    // headers = curl_slist_append(headers, "Custom-Header: Value");
    // curl_easy_setopt(curl_session, CURLOPT_HTTPHEADER, headers);

    /* execute connection request */
    CURLcode res_code = curl_easy_perform(curl_session);

    /* validate */
    if (res_code != CURLE_OK) {
      finalize_ws_connection(session_id);
      log_fatal("%s on session_id[%d] with error: %s\n", "Failed to establish WebSocket connection", session_id, curl_easy_strerror(res_code)); // #FIXME no internet fatal
      return NULL_CURL_SESSION;
    }

    log_info("%s\n", "WebSocket connection established.");
    return session_id;
  }
private:
/* 
  write_callback method, 
    - on receiving a message it will add the data to the session queue. 
    - there is a separate thread that process those queues, the writeback is left simple without the logic of intrepreting the data.
*/
  static size_t websocket_input_callback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    /* interpret the session_id */
    ws_session_id_t session_id = *(ws_session_id_t*)userdata;
      
    /* lock the current session mutex */
    LOCK_GUARD(*WebsocketAPI::get_session_mutex(session_id));

    /* push the received input frame to the session_id FIFO queue */
    ws_incomming_data frame_to_queue;
    {
      frame_to_queue.data = std::string(ptr, size * nmemb);
      frame_to_queue.local_timestamp = std::chrono::system_clock::now();
    }
    WebsocketAPI::get_session_input_queue(session_id)->push(frame_to_queue);

    log_dbg("%s in session_id[%d] : %s\n", "Websocket callback received message: ", session_id, frame_to_queue.data.c_str());

    /* return the number of processed bytes, in this case we return the total count */
    return size * nmemb;
  }
/* 
  send message 
*/
  static long write_ws_message(ws_session_id_t session_id, ws_outgoing_data frame_to_queue) {
    LOCK_GUARD(*WebsocketAPI::get_session_mutex(session_id));
    get_session_output_queue(session_id)->push(frame_to_queue);
    return 0x0;
  }
};
ENFORCE_SINGLETON_DESIGN(WebsocketAPI);
int WebsocketAPI::sessions_counter = 0;
std::mutex WebsocketAPI::global_ws_mutex;
std::unordered_map<ws_session_id_t, CURL*> WebsocketAPI::curl_ws_sessions;
std::unordered_map<ws_session_id_t, std::mutex> WebsocketAPI::session_mutex;

WebsocketAPI::_init WebsocketAPI::_initializer;
} /* namespace curl */
} /* namespace camahjucunu */
} /* namespace cuwacunu */