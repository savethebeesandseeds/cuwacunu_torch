#pragma once

/* CURL implementing RFC 6455, we just use CURL */

#include "piaabo/dutils.h"
#include "piaabo/architecture.h"
#include "camahjucunu/https/curl_toolkit/curl_utils.h"
#include <thread>
#include <chrono>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <condition_variable>

RUNTIME_WARNING("(curl_websocket_api.h)[] fatal error on unknown session_id (fatal might be a good thing here there shodun't be a reason to allow undefined instructions).\n");
RUNTIME_WARNING("(curl_websocket_api.h)[ws_incomming_data_t] not necesarly local_timestamp matches the timestamps in the body of the responces. \n");
RUNTIME_WARNING("(curl_websocket_api.h)[] writing to dbg might be slow if dbg is checking config every time.\n");
RUNTIME_WARNING("(curl_websocket_api.h)[] TX_queue for orders, might delay the sending of instructions, so include time_window in the instruction.\n");
RUNTIME_WARNING("(curl_websocket_api.h)[] many curl options are uncommented, this needs to be reviewed.\n");
RUNTIME_WARNING("(curl_websocket_api.h)[ws_write] ws_write should return the message id.\n");
RUNTIME_WARNING("(curl_websocket_api.h)[ws_write] add encoding support for ws_write_text.\n");
RUNTIME_WARNING("(curl_websocket_api.h)[] log the times and redirection count (curl_easy_getinfo()).\n");


#define WS_NORMAL_TERMINATION                 (uint16_t) 0x3E8  /* Normal closure; the connection successfully completed whatever purpose for which it was created. */
#define WS_GOING_AWAY_TERMINATION             (uint16_t) 0x3E9  /* Going away; the server is going down, or the browser is navigating away from the page that opened the connection. */
#define WS_PROTOCOL_ERROR_TERMINATION         (uint16_t) 0x3EA  /* Protocol error; the endpoint is terminating the connection due to a protocol error. */
#define WS_UNSUPORTED_DATA_TERMINATION        (uint16_t) 0x3EB  /* Unsupported data; the endpoint received data of a type it cannot accept (for example, a text-only endpoint receiving binary data). */
#define WS_INVALID_FRAME_TERMINATION          (uint16_t) 0x3EF  /* Invalid frame payload data; the endpoint received data inconsistent with the type of the message (e.g., non-UTF-8 data within a text message). */
#define WS_INTERNAL_SERVER_ERROR_TERMINATION  (uint16_t) 0x3F3  /* Internal server error; the server is terminating the connection due to a problem it encountered. */

namespace cuwacunu {
namespace camahjucunu {
namespace curl {

using ws_session_id_t = int;
using ws_WriteCallback_fn = size_t (*)(char*, size_t, size_t, void*);

struct ws_incomming_data_t {
  long id;
  std::string data;
  std::chrono::system_clock::time_point local_timestamp;
};
ENFORCE_ARCHITECTURE_DESIGN(ws_incomming_data_t);

struct ws_outgoing_data_t {
  long    frame_id;
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
    - map to a queue of received data, one per session
    - map to a queue of outcomming data, one per session
    - global (in the context of WebsocketAPI) mutex
    - sessions_counter; to create unique ids for every session
    - session thread for the RX (incomming) events
    - session thread for the TX (outgoing) events
    - global conditional trigger variable
*/
  static std::unordered_map<ws_session_id_t, int> curl_ws_session_still_running;
  static std::unordered_map<ws_session_id_t, CURL*> curl_ws_sessions;
  static std::unordered_map<ws_session_id_t, CURLM *> curl_multi_handles;
  static std::unordered_map<ws_session_id_t, ws_session_id_t> id_sessions;  /* this unusual queue is to preserve in memory a reference to the ws_session_id when passed to the curl contructor */
  static std::unordered_map<ws_session_id_t, std::queue<ws_incomming_data_t>> session_RX_frames_queue;
  static std::unordered_map<ws_session_id_t, std::queue<ws_outgoing_data_t>> session_TX_frames_queue;
  static std::unordered_map<ws_session_id_t, std::mutex> session_mutex;
  static std::unordered_map<ws_session_id_t, std::thread> session_RX_thread;
  static std::unordered_map<ws_session_id_t, std::thread> session_TX_thread;
  static std::mutex global_ws_mutex;
  static int sessions_counter;
  static std::condition_variable global_ws_trigger;

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
    log_info("Initializing WebsocketAPI \n");

    LOCK_GUARD(WebsocketAPI::global_ws_mutex);

    /* make sure curl is globaly initialized */
    dcurl_global_init();

    /* trigger a cleanup process at the end */
    std::atexit(WebsocketAPI::finit);
  }
  static void finit() {
    log_info("Finalizing WebsocketAPI \n");
    
    LOCK_GUARD(WebsocketAPI::global_ws_mutex);

    /* do a curl global cleanup */
    dcurl_global_cleanup();
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
  static std::queue<ws_incomming_data_t>* get_session_RX_queue(ws_session_id_t session_id) {
    /* valdiate */
    if (session_id == NULL_CURL_SESSION || WebsocketAPI::session_RX_frames_queue.find(session_id) == WebsocketAPI::session_RX_frames_queue.end()) {
      log_fatal("%s with id: %d\n", "Failed to identify websocket session mutex", session_id);
      return nullptr;
    }
    /* retrive session mutex */
    return &(WebsocketAPI::session_RX_frames_queue.at(session_id));
  }
  static std::queue<ws_outgoing_data_t>* get_session_TX_queue(ws_session_id_t session_id) {
    /* valdiate */
    if (session_id == NULL_CURL_SESSION || WebsocketAPI::session_TX_frames_queue.find(session_id) == WebsocketAPI::session_TX_frames_queue.end()) {
      log_fatal("%s with id: %d\n", "Failed to identify websocket session mutex", session_id);
      return nullptr;
    }
    /* retrive session mutex */
    return &(WebsocketAPI::session_TX_frames_queue.at(session_id));
  }
  static void remove_session(ws_session_id_t session_id) {
    /* free memory */
    {
      LOCK_GUARD(*WebsocketAPI::get_session_mutex(session_id));
      WebsocketAPI::curl_ws_sessions.erase(session_id);
      WebsocketAPI::curl_multi_handles.erase(session_id);
      WebsocketAPI::session_RX_frames_queue.erase(session_id);
      WebsocketAPI::session_RX_thread.erase(session_id);
      WebsocketAPI::session_TX_thread.erase(session_id);
      WebsocketAPI::curl_ws_session_still_running.erase(session_id);
      /* instead of errasing the reference session_id, we market it null to indicate this is no longer a valid session */
      WebsocketAPI::id_sessions.at(session_id) = NULL_CURL_SESSION;
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
    
    /* initialize the multi init object */
    CURLM * new_curl_multi_handle = curl_multi_init();

    /* create the session variables */
    ws_session_id_t new_session_id = WebsocketAPI::sessions_counter;
    std::queue<ws_incomming_data_t> new_session_RX_frames_queue;
    std::queue<ws_outgoing_data_t> new_session_TX_frames_queue;
    WebsocketAPI::sessions_counter ++;

    /* add the session variables to the session queues */
    WebsocketAPI::curl_ws_sessions.emplace(new_session_id, new_curl_session);
    WebsocketAPI::curl_multi_handles.emplace(new_session_id, new_curl_multi_handle);
    WebsocketAPI::id_sessions.emplace(new_session_id, new_session_id);
    WebsocketAPI::curl_ws_session_still_running.emplace(new_session_id, 0);
    WebsocketAPI::session_RX_frames_queue.emplace(new_session_id, new_session_RX_frames_queue);
    WebsocketAPI::session_TX_frames_queue.emplace(new_session_id, new_session_TX_frames_queue);
    WebsocketAPI::session_mutex.emplace(std::piecewise_construct, std::forward_as_tuple(new_session_id), std::tuple<>());

    /* launch the flush TX messages (outgoing messages) thread */
    std::thread new_flush_TX_queue_thread(WebsocketAPI::flush_messages_loop, new_session_id);
    WebsocketAPI::session_TX_thread.emplace(new_session_id, std::move(new_flush_TX_queue_thread));
    WebsocketAPI::session_TX_thread.at(new_session_id).detach();

    /* log */
    log_info("New websocket session created with session_id[%d].\n", 
      new_session_id);

    return new_session_id;
  }
public:
/*
  Wait to flush
    - waits until session_id TX queue has been flushed
*/
  static void ws_wait_to_flush(ws_session_id_t session_id) {
    // std::mutex aux_mutex;
    std::unique_lock<std::mutex> lock(WebsocketAPI::global_ws_mutex);  
    global_ws_trigger.wait(lock, [session_id] { 
      return WebsocketAPI::get_session_TX_queue(session_id)->empty();
    });
  }
/*
  Wait for curl loop to finish
    - waits until the curl loop has finished
*/
  static void ws_wait_loop_to_finish(ws_session_id_t session_id) {
    // std::mutex aux_mutex;
    std::unique_lock<std::mutex> lock(WebsocketAPI::global_ws_mutex);  
    global_ws_trigger.wait(lock, [session_id] { 
      return WebsocketAPI::curl_ws_session_still_running.at(session_id) == 0; 
    });
  }
public:
/* 
  Conextion method (public)
    - finalize connection
*/
  static void ws_finalize(ws_session_id_t session_id) {
    log_info("%s with session_id[%d]...\n", "Finalizing WebSocket connection", session_id);

    /* close session */
    long close_message_id = ws_write_close(session_id, WS_NORMAL_TERMINATION);
    
    /* wait untill the RX queue is flushed */
    WebsocketAPI::ws_wait_to_flush(session_id);

    /* mark the end of the session validity */
    WebsocketAPI::id_sessions.at(session_id) = NULL_CURL_SESSION;
    
    /* wait until the curl_loop is done */
    WebsocketAPI::ws_wait_loop_to_finish(session_id);
    
    /* terminate session */
    {
      /* lock */
      LOCK_GUARD(*WebsocketAPI::get_session_mutex(session_id));

      /* cleanup from the curl-multi-object */
      curl_multi_remove_handle(
          WebsocketAPI::curl_multi_handles.at(session_id), 
          WebsocketAPI::get_session(session_id));

      /* cleanup the curl_multi handle */
      curl_multi_cleanup(WebsocketAPI::curl_multi_handles.at(session_id));
      
      /* cleanup (free) memory */
      curl_easy_cleanup(WebsocketAPI::get_session(session_id));
    } /* unlock */
    
    /* finalize session */
    WebsocketAPI::remove_session(session_id);

    log_info("%s with session_id[%d] message_id[%ld].\n", 
      "[success] Finalized WebSocket connection", session_id, close_message_id);
  }
/* 
  Conextion method (public)
    - initialize connection
*/
public:
  static ws_session_id_t ws_init(const std::string& url) {
    /* create curl session */
    ws_session_id_t session_id = WebsocketAPI::initialize_curl_ws_session();
    if(session_id == NULL_CURL_SESSION) { return NULL_CURL_SESSION; }

    /* get curl session from the session_id */
    CURL* curl_session = WebsocketAPI::get_session(session_id);
    /* skiping curl_session validation */
    
    /* configure curl session for websockets */
    curl_easy_setopt(curl_session, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_session, CURLOPT_WRITEFUNCTION , WebsocketAPI::websocket_RX_callback);
    curl_easy_setopt(curl_session, CURLOPT_WRITEDATA, &(WebsocketAPI::id_sessions.at(session_id)));
    curl_easy_setopt(curl_session, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl_session, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl_session, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl_session, CURLOPT_CONNECTTIMEOUT, 5L);

    curl_easy_setopt(curl_session, CURLOPT_VERBOSE, 1L);

    /* // Optional setup options, to be reviewd
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

    /* add the session to the curl-multi-object */
    {
      LOCK_GUARD(WebsocketAPI::global_ws_mutex);
    
      curl_multi_add_handle(WebsocketAPI::curl_multi_handles.at(session_id), curl_session); /* please only add one session on each multi_handle */
    }

    /* Launch a thread the curl loop to run on this session */
    std::thread new_curl_thread(WebsocketAPI::curl_loop, session_id);

    /* detach the new new_curl_thread */
    new_curl_thread.detach();

    /* since the curl is detached, we wait for a successfull schema change */
    std::mutex aux_mutex; std::unique_lock<std::mutex> lock(aux_mutex);

    global_ws_trigger.wait(lock, [session_id] {
      char *scheme = NULL;
      if (curl_easy_getinfo(
            WebsocketAPI::curl_ws_sessions.at(session_id), 
            CURLINFO_SCHEME, &scheme) == CURLE_OK) 
      {
        if(scheme && (strcmp(scheme, "WS") == 0 || strcmp(scheme, "WSS") == 0)) {
          log_dbg("Scheme change detected on session_id[%d] \n", session_id);
          /* continue */
          return true;
        }
      }

      /* wait */
      return false;
    });

    /* after waiting */
    log_info("session_id[%d] : %s\n", 
      session_id, "WebSocket connection established succesfully.");
    
    return session_id;
  }
public:
/* 
  send message:
    - push a message to the TX queue
*/
  static long ws_write_close(ws_session_id_t session_id, unsigned short closing_code) {
    /* create the object */
    ws_outgoing_data_t frame_to_queue;
    {
      unsigned short close_code = ws_htons(closing_code); /* network byte order */
      /* fill the frame object */
      frame_to_queue.frame_data.resize(sizeof(close_code));
      memcpy(frame_to_queue.frame_data.data(), &close_code, sizeof(close_code));
      frame_to_queue.frame_size = frame_to_queue.frame_data.size();
      frame_to_queue.frame_type = CURLWS_CLOSE;
      frame_to_queue.frame_id = 0x0;
      frame_to_queue.local_timestamp = std::chrono::system_clock::now();
    }
    /* push to queue */
    {
      LOCK_GUARD(*WebsocketAPI::get_session_mutex(session_id));
      WebsocketAPI::get_session_TX_queue(session_id)->push(frame_to_queue);
    }
    WebsocketAPI::global_ws_trigger.notify_all();
    return frame_to_queue.frame_id;
  }
  static long ws_write_binary(ws_session_id_t session_id, const std::vector<unsigned char>& data) {
    /* create the object */
    ws_outgoing_data_t frame_to_queue;
    {
      /* fill the frame object */
      frame_to_queue.frame_size = data.size();
      frame_to_queue.frame_data = std::move(data);
      frame_to_queue.frame_type = CURLWS_BINARY;
      frame_to_queue.frame_id = 0x0;
      frame_to_queue.local_timestamp = std::chrono::system_clock::now();
    }
    /* push to queue */
    {
      LOCK_GUARD(*WebsocketAPI::get_session_mutex(session_id));
      WebsocketAPI::get_session_TX_queue(session_id)->push(std::move(frame_to_queue));
    }
    WebsocketAPI::global_ws_trigger.notify_all();
    return frame_to_queue.frame_id;
  }
  static long ws_write_text(ws_session_id_t session_id, std::string data) {
    /* create the object */
    ws_outgoing_data_t frame_to_queue;
    {
      std::vector<unsigned char> data_vect(data.begin(), data.end()); /* ACSII encoding asumption */
      /* fill the frame object */
      frame_to_queue.frame_data = data_vect;
      frame_to_queue.frame_id   = 0x0;
      frame_to_queue.frame_type = CURLWS_TEXT;
      frame_to_queue.frame_size = data.size();
      frame_to_queue.local_timestamp = std::chrono::system_clock::now();
    }
    /* push to queue */
    {
      LOCK_GUARD(*WebsocketAPI::get_session_mutex(session_id));
      WebsocketAPI::get_session_TX_queue(session_id)->push(std::move(frame_to_queue));
    }
    WebsocketAPI::global_ws_trigger.notify_all();
    return frame_to_queue.frame_id;
  }
private:
/*
  flush (TX) messages
    - runs an independent thread for every session_id
    - trigger (notify) when on WebsocketAPI::ws_write_* methods
*/
  static void flush_messages_loop(ws_session_id_t session_id) {
    
    do {
      std::queue<ws_outgoing_data_t>* queue = WebsocketAPI::get_session_TX_queue(session_id);
      /* retrive the TX queue */

      /* wait until TX queue is not empty */
      std::mutex aux_mutex;
      std::unique_lock<std::mutex> lock(aux_mutex);
      global_ws_trigger.wait_for(lock, std::chrono::seconds(5), [queue, session_id]() { 
        return WebsocketAPI::id_sessions.at(session_id) == NULL_CURL_SESSION || !queue->empty(); 
      });

      /* check to break the thread loop and finalize */
      if(WebsocketAPI::id_sessions.at(session_id) == NULL_CURL_SESSION) {
        log_dbg("Stop flushing messages for session_id[%d]\n", 
          session_id);
        break;
      }

      /* flush the queue */
      while( WebsocketAPI::id_sessions.at(session_id) != NULL_CURL_SESSION && !queue->empty()) {
        /* retrive the first element of the queue */
        ws_outgoing_data_t frame_from_queue = std::move(queue->front()); /* FIFO */

        /* send the message */
        CURLcode res = send_ws_frame(
          WebsocketAPI::get_session(session_id),
          frame_from_queue.frame_data.data(),
          frame_from_queue.frame_size,
          frame_from_queue.frame_type
        );

        /* validate the response */
        if (res != CURLE_OK) {
          log_err("Unable to send frame_id[%ld] from session_id[%d], with error: %s\n", 
            frame_from_queue.frame_id, session_id, curl_easy_strerror(res));
        }

        /* remove the queue element */
        queue->pop();
      }
      
      /* unlock the session mutex and notify trigger */
      WebsocketAPI::global_ws_trigger.notify_all();

    } while(true); /* low CPU consuption loop, await handled by the trigger variable */
  }
private:
/* curl_multi... loop */
  static void curl_loop(ws_session_id_t session_id) {
    log_dbg("Dispatching a new curl-thread on session_id[%d].\n", session_id);
    bool notified_scheme_change = false;
    int numfds;
    CURLMcode res_code;

    do {
      /* check session is still active */
      if(WebsocketAPI::id_sessions.at(session_id) == NULL_CURL_SESSION) {break;}

      /* Perform the next action in curl */
      if (
        (res_code = curl_multi_perform(
          WebsocketAPI::curl_multi_handles.at(session_id), 
          &WebsocketAPI::curl_ws_session_still_running.at(session_id)))
        != CURLM_OK)
      {
        log_fatal("Failed to perform curl_multi operation with error: %s\n",
          curl_multi_strerror(res_code));
        return;
      }

      /* check session is still active */
      if(WebsocketAPI::id_sessions.at(session_id) == NULL_CURL_SESSION) {break;}

      /* Wait for activity or timeout */
      if (
        (res_code = curl_multi_wait(
          WebsocketAPI::curl_multi_handles.at(session_id), 
          NULL, 0, 1000, &numfds))
        != CURLM_OK)
      {
        log_err("curl_multi_wait() failed: %s\n", 
          curl_multi_strerror(res_code));
        break;
      }

      /* check session is still active */
      if(WebsocketAPI::id_sessions.at(session_id) == NULL_CURL_SESSION) {break;}

      /* verify connetion failure, overall verify errors */
      {
        CURLMsg *msg;
        int msgs_left;
        while ((msg = curl_multi_info_read(WebsocketAPI::curl_multi_handles.at(session_id), &msgs_left))) {
          if (msg->msg == CURLMSG_DONE) {
            if (msg->data.result == CURLE_COULDNT_RESOLVE_HOST) {
              log_fatal("Curl failed to resolve host (no internet).\n");
            } else if (msg->data.result == CURLE_COULDNT_CONNECT) {
              log_fatal("Curl failed to connect or shutting down connection.\n");
            } else if (msg->data.result != CURLE_OK) {
              log_fatal("Curl frror: %s\n", curl_easy_strerror(msg->data.result));
            }
          }
        }
      }

      /* check session is still active */
      if(WebsocketAPI::id_sessions.at(session_id) == NULL_CURL_SESSION) {break;}

      /* verify scheme change */
      if (!notified_scheme_change) {
        char *scheme = NULL;
        if (curl_easy_getinfo(
              WebsocketAPI::curl_ws_sessions.at(session_id), 
              CURLINFO_SCHEME, &scheme) == CURLE_OK) 
        {
          if(scheme && (strcmp(scheme, "WS") == 0 || strcmp(scheme, "WSS") == 0)) {
            
            /* validate the response code is 101 (switch protocol) */
            long response_code;
            CURLcode res = curl_easy_getinfo(
              WebsocketAPI::curl_ws_sessions.at(session_id), 
              CURLINFO_RESPONSE_CODE, &response_code);
            
            if(res == CURLE_OK && response_code == 101) {
              
              /* notify schema has changed to Websocket */
              notified_scheme_change = true;
              WebsocketAPI::global_ws_trigger.notify_all();

            }
          }
        }
      }

      /* check session is still active */
      if(WebsocketAPI::id_sessions.at(session_id) == NULL_CURL_SESSION) {break;}

    } while (WebsocketAPI::curl_ws_session_still_running.at(session_id) > 0);

    /* Curl ran out of jobs */
    FLUSH_SYS_ERR(); /* curl triggers some errors that are not critical */
    log_info("curl-thread session_id[%d] finished operating.\n", session_id);

    WebsocketAPI::curl_ws_session_still_running.at(session_id) = 0;
    WebsocketAPI::global_ws_trigger.notify_all();
    
    return;
  }
private:
/* 
  write_callback method, 
    - on receiving a message it will add the data to the session queue. 
    - there is a separate thread that process those queues, the writeback is left simple without the logic of intrepreting the data.
*/
  static size_t websocket_RX_callback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    /* interpret the session_id */
    ws_session_id_t session_id = *(ws_session_id_t*)userdata;
    ws_incomming_data_t frame_to_queue;
      
    /* lock the current session mutex */
    {
      LOCK_GUARD(*WebsocketAPI::get_session_mutex(session_id));

      /* push the received RX frame to the session_id FIFO queue */
      {
        frame_to_queue.data = std::string(ptr, size * nmemb);
        frame_to_queue.local_timestamp = std::chrono::system_clock::now();
      }
      WebsocketAPI::get_session_RX_queue(session_id)->push(frame_to_queue);

    } /* unlock */

    log_dbg("%s in session_id[%d] : %s\n", "Websocket callback received message: ", 
      session_id, frame_to_queue.data.c_str());

    /* return the number of processed bytes, in this case we return the total count */
    return size * nmemb;
  }
};
ENFORCE_SINGLETON_DESIGN(WebsocketAPI);

std::condition_variable WebsocketAPI::global_ws_trigger;
int WebsocketAPI::sessions_counter = 0;
std::mutex WebsocketAPI::global_ws_mutex;
std::unordered_map<ws_session_id_t, CURL*> WebsocketAPI::curl_ws_sessions;
std::unordered_map<ws_session_id_t, CURLM *> WebsocketAPI::curl_multi_handles;
std::unordered_map<ws_session_id_t, int> WebsocketAPI::curl_ws_session_still_running;
std::unordered_map<ws_session_id_t, std::mutex> WebsocketAPI::session_mutex;
std::unordered_map<ws_session_id_t, std::thread> WebsocketAPI::session_RX_thread;
std::unordered_map<ws_session_id_t, std::thread> WebsocketAPI::session_TX_thread;
std::unordered_map<ws_session_id_t, std::queue<ws_incomming_data_t>> WebsocketAPI::session_RX_frames_queue;
std::unordered_map<ws_session_id_t, std::queue<ws_outgoing_data_t>> WebsocketAPI::session_TX_frames_queue;
std::unordered_map<ws_session_id_t, ws_session_id_t> WebsocketAPI::id_sessions;
  
WebsocketAPI::_init WebsocketAPI::_initializer;

} /* namespace curl */
} /* namespace camahjucunu */
} /* namespace cuwacunu */