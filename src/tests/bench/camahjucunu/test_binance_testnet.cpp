#include "piaabo/dutils.h"
#include "camahjucunu/https/curl_toolkit/websockets_api/curl_websocket_api.h"

int main() {
    const char* websocket_url = "wss://testnet.binance.vision/ws-api/v3";

    cuwacunu::camahjucunu::curl::ws_session_id_t session_id;
    
    session_id = cuwacunu::camahjucunu::curl::WebsocketAPI::ws_init(websocket_url);

    cuwacunu::camahjucunu::curl::WebsocketAPI::ws_write_ping(session_id);
    cuwacunu::camahjucunu::curl::WebsocketAPI::ws_write_pong(session_id);

    cuwacunu::camahjucunu::curl::WebsocketAPI::ws_write_text(session_id, "{\"id\":\"1\", \"method\":\"ping\"}");
    cuwacunu::camahjucunu::curl::WebsocketAPI::ws_write_text(session_id, "{\"id\":\"2\", \"method\":\"ping\"}");
    cuwacunu::camahjucunu::curl::WebsocketAPI::ws_write_text(session_id, "{\"id\":\"3\", \"method\":\"ping\"}");
    cuwacunu::camahjucunu::curl::WebsocketAPI::ws_write_text(session_id, "{\"id\":\"4\", \"method\":\"ping\"}");
    
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    cuwacunu::camahjucunu::curl::WebsocketAPI::ws_write_text(session_id, "{\"id\":\"FA\", \"method\":\"klines\", \"params\": {\"symbol\":\"BTCTUSD\",\"interval\":\"1s\"}}");
    cuwacunu::camahjucunu::curl::WebsocketAPI::ws_write_text(session_id, "{\"id\":\"FB\", \"method\":\"klines\", \"params\": {\"symbol\":\"BTCTUSD\",\"interval\":\"1s\"}}");
    cuwacunu::camahjucunu::curl::WebsocketAPI::ws_write_text(session_id, "{\"id\":\"FF\", \"method\":\"klines\", \"params\": {\"symbol\":\"BTCTUSD\",\"interval\":\"1s\"}}");

    std::this_thread::sleep_for(std::chrono::seconds(60*4)); /* wait until there is a ping comming from binance */

    cuwacunu::camahjucunu::curl::WebsocketAPI::ws_finalize(session_id);

    return 0;
}