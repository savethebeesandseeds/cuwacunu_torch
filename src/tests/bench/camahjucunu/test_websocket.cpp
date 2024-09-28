#include "piaabo/dutils.h"
#include "camahjucunu/https/curl_toolkit/websockets_api/curl_websocket_api.h"
// #include "camahjucunu/https/curl_toolkit/curl_utils.h"

int main() {
    const char* websocket_url = "wss://echo.websocket.org/";

    cuwacunu::camahjucunu::curl::ws_session_id_t session_id; 
    
    session_id = cuwacunu::camahjucunu::curl::WebsocketAPI::ws_init(websocket_url);

    cuwacunu::camahjucunu::curl::WebsocketAPI::ws_write_text(session_id, "message1");
    
    cuwacunu::camahjucunu::curl::WebsocketAPI::ws_write_text(session_id, "message2");

    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    cuwacunu::camahjucunu::curl::WebsocketAPI::ws_finalize(session_id);

    return 0;
}