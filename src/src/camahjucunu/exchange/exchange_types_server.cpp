#include "camahjucunu/exchange/exchange_types_server.h"

namespace cuwacunu {
namespace camahjucunu {
namespace exchange {
/* --- --- --- --- --- --- --- --- --- --- --- */
/*            arguments structures             */
/* --- --- --- --- --- --- --- --- --- --- --- */
std::string     ping_args_t::jsonify() { return "{}"; }
std::string     time_args_t::jsonify() { return "{}"; }

/* --- --- --- --- --- --- --- --- --- --- --- */
/*         expected return structures          */
/* --- --- --- --- --- --- --- --- --- --- --- */
ping_ret_t::ping_ret_t (const std::string &json) { deserialize(*this, json); };
time_ret_t::time_ret_t (const std::string &json) { deserialize(*this, json); };

/* --- --- --- --- --- --- --- --- --- --- --- */
/*         deserialize specializations         */
/* --- --- --- --- --- --- --- --- --- --- --- */

void deserialize(ping_ret_t& deserialized, const std::string& json) {
  /*
    {
      "id": "922bcc6e-9de8-440d-9e84-7c80933a8d0d",
      "status": 200,
      "result": {},
      "rateLimits": [
        {
          "rateLimitType": "REQUEST_WEIGHT",
          "interval": "MINUTE",
          "intervalNum": 1,
          "limit": 6000,
          "count": 1
        }
      ]
    }
  */
  
  /* parse the json string */
  INITIAL_PARSE(json, root, root_obj);
  
  /* frame response object */
  ASSIGN_STRING_FIELD_FROM_JSON_STRING(deserialized.frame_rsp, root_obj, "id", frame_id);
  ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(deserialized.frame_rsp, root_obj, "status", http_status, unsigned int);
  
  /* no other fields for ping_ret_t */
}

void deserialize(time_ret_t& deserialized, const std::string &json) {
  /*
    {
      "id": "187d3cb2-942d-484c-8271-4e2141bbadb1",
      "status": 200,
      "result": {
        "serverTime": 1656400526260
      },
      "rateLimits": [
        {
          "rateLimitType": "REQUEST_WEIGHT",
          "interval": "MINUTE",
          "intervalNum": 1,
          "limit": 6000,
          "count": 1
        }
      ]
    }
  */

  /* parse the json string */
  INITIAL_PARSE(json, root, root_obj);
  RETRIVE_OBJECT(root_obj, "result", result_obj);
  
  /* frame response object */
  ASSIGN_STRING_FIELD_FROM_JSON_STRING(deserialized.frame_rsp, root_obj, "id", frame_id);
  ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(deserialized.frame_rsp, root_obj, "status", http_status, unsigned int);
  
  /* result fields */
  ASSIGN_NUMBER_FIELD_FROM_JSON_NUMBER(deserialized, result_obj, "serverTime", serverTime, long);

}

} /* namespace exchange */
} /* namespace cuwacunu */
} /* namespace camahjucunu */
