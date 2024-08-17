#include "piaabo/dutils.h"

cleanup_ws_connection
cleanup_curl
send_ws_message
receive_ws_message
handle_event
    open
    close
    message



// Pseudocode for a WebSocket client

// Define the WebSocket URL
websocket_url = "ws://example.com/socket"

// Function to create and open a WebSocket connection
function openWebSocket(url):
    // Initialize the WebSocket object
    websocket = new WebSocket(url)

    // Event handler for when the connection is opened
    websocket.onopen = function():
        print("Connection established")
        // Send a message through the WebSocket
        sendMessage("Hello, server!")

    // Event handler for receiving messages from the server
    websocket.onmessage = function(message):
        print("Received message: " + message.data)
        // Handle message based on content or type
        handleMessage(message.data)

    // Event handler for errors
    websocket.onerror = function(error):
        print("Error occurred: " + error.message)

    // Event handler for when the connection is closed
    websocket.onclose = function():
        print("Connection closed")

// Function to send a message through the WebSocket
function sendMessage(message):
    if websocket.readyState == WebSocket.OPEN:
        websocket.send(message)
    else:
        print("WebSocket is not open")

// Function to handle incoming messages (you can customize this part)
function handleMessage(data):
    // Process the message based on your application's logic
    print("Processing message: " + data)

// Main execution flow
openWebSocket(websocket_url)










#include <curl/curl.h>
#include <curl/curl.h>
#include <iostream>
#include <string>

#include "piaabo/dutils.h"


RUNTIME_WARNING("(curl_websocket_api.h)[] fix no internet causes fatal error on initialization.\n");


... implement in singleton with std::vector<CURL *>



bool GLOBAL_CURL_INIT = false;

void curl_global_init() {
    if(GLOBAL_CURL_INIT) {
        log_warn("Repeated curl_global_init().\n");
        return;
    }
    // Initialize global curl environment
    CURLcode res = curl_global_init(CURL_GLOBAL_ALL);
    if (res != CURLE_OK) {
        log_fatal("Failed to initialize curl: %s\n", curl_easy_strerror(res));
        return;
    }

    GLOBAL_CURL_INIT = true;
}


// Function to initialize and open a WebSocket connection
CURL* initialize_curl_ws_session(const std::string& url) {
    if(GLOBAL_CURL_INIT == false) {
        log_warn("initialize_curl_ws_session without initializing, forcing initialization and continuing as expected.\n");
        curl_global_init();
    }
    // Create a CURL handle for the connection
    CURL* curl_session = curl_easy_init();
    if (!curl_session) {
        std::cerr << "Failed to create CURL handle." << std::endl;
        return nullptr;
    }

    return curl_session;  // Return the handle to be used for sending/receiving messages
}

CURL* curl_session initialize_ws_connection(const std::string& url) {
    // Initialize curl_session
    CURL* curl_session = initialize_curl_ws_curl(const std::string& url);
    if(curl_session == nullptr) { return nullptr; }
    // Set the URL for the WebSocket
    curl_easy_setopt(curl_session, CURLOPT_URL, url.c_str());
    // Attempt to upgrade the connection to WebSocket
    CURLcode res = curl_easy_perform(curl_session);
    if (res != CURLE_OK) {
        std::cerr << "Failed to establish WebSocket connection: " << curl_easy_strerror(res) << std::endl;
        curl_easy_cleanup(curl_session);
        return nullptr;
    }

    std::cout << "WebSocket connection established." << std::endl;
}


// Function to send a close frame to the WebSocket server
void finalize_curl_ws_session(CURL* curl_session) {
    // WebSocket close frame bytes (this is a standard close frame)
    const char close_frame[2] = {0x88, 0x00}; // 0x88 is the close opcode, 0x00 means no payload

    // Send the close frame
    size_t bytes_sent = 0;
    CURLcode res = curl_easy_send(curl_session, close_frame, sizeof(close_frame), &bytes_sent);
    if (res != CURLE_OK) {
        std::cerr << "Failed to send close frame: " << curl_easy_strerror(res) << std::endl;
    } else {
        std::cout << "Close frame sent." << std::endl;
    }
}


// Function to clean up the WebSocket connection
void cleanup_ws_connection(CURL* curl_session) {
    if (curl_session == nullptr) {
        log_warn("attempt to cleanup a null curl session. Excaping cleaup proceedure.\n");
        return;
    }
    // Send finalization message
    finalize_curl_ws_session(CURL* curl_session)
    // Close the CURL handle
    curl_easy_cleanup(curl_session);
    std::cout << "WebSocket connection cleaned up." << std::endl;
}

void curl_global_cleanup() {
    // Clean up CURL for this thread
    curl_global_cleanup();
}












bool GLOBAL_CURL_INIT = false;

void curl_global_init() {
    
}


#include <vector>
#include <mutex>


namespace cuwacunu {
namespace camahjucunu {
namespace curl {

struct WebsocketAPI {
public:
    static std::vector<CURL*> unthreaded_curl;
    static std::mutex global_lock;
    static bool global_initialized = false;
public:
  static struct _init {public:_init(){WebsocketAPI::init();}}_initializer;
	static void init()
	{
        std::lock_guard<std::mutex> lock(mtx);
        if(WebsocketAPI::global_initialized) {
            log_warn("Repeated curl_global_init(). Skipping and continuing as expected.\n");
            return;
        }
        CURLcode res = curl_global_init(CURL_GLOBAL_ALL);
        if (res != CURLE_OK) {
            log_fatal("Failed to initialize curl: %s\n", curl_easy_strerror(res)); // #FIXME no internet fatal
            return;
        }
        WebsocketAPI::global_initialized = true;
    }
}

WebsocketAPI::_init WebsocketAPI::_initializer;
std::mutex WebsocketAPI::global_lock;

} /* namespace curl */
} /* namespace camahjucunu */
} /* namespace cuwacunu */

