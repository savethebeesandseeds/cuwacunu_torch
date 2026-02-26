#include "piaabo/dutils.h"
#include "piaabo/https_compat/curl_toolkit/websockets_api/curl_websocket_api.h"
// #include "piaabo/https_compat/curl_toolkit/curl_utils.h"

int main() {
    const char* websocket_url = "wss://echo.websocket.org/";

    cuwacunu::piaabo::curl::ws_session_id_t session_id; 
    
    session_id = cuwacunu::piaabo::curl::WebsocketAPI::ws_init(websocket_url);

    cuwacunu::piaabo::curl::WebsocketAPI::ws_write_text(session_id, "message1");
    
    cuwacunu::piaabo::curl::WebsocketAPI::ws_write_text(session_id, "message2");

    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    cuwacunu::piaabo::curl::WebsocketAPI::ws_finalize(session_id);

    return 0;
}
