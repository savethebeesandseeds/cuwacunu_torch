#include <cassert>
#include <string>

#include "piaabo/dutils.h"
#include "piaabo/djson_parsing.h"

using namespace cuwacunu;
using namespace piaabo;

static void expect_parse_failure(const std::string& json) {
  bool failed = false;
  try {
    JsonParser parser(json);
    (void)parser.parse();
  } catch (const std::runtime_error&) {
    failed = true;
  }
  assert(failed);
}

void runTests() {
  {
    const std::string json = R"({"name":"John","age":25})";
    JsonValue root = JsonParser(json).parse();
    assert(root.type == JsonValueType::OBJECT);
    auto& obj = *root.objectValue;
    assert(obj.at("name").type == JsonValueType::STRING);
    assert(obj.at("name").stringValue == "John");
    assert(obj.at("age").type == JsonValueType::NUMBER);
    assert(obj.at("age").numberValue == 25.0);
  }

  {
    const std::string json = R"({"person":{"name":"Alice","scores":[85,90,92]}})";
    JsonValue root = JsonParser(json).parse();
    auto& person = *root.objectValue->at("person").objectValue;
    assert(person.at("name").stringValue == "Alice");
    auto& scores = *person.at("scores").arrayValue;
    assert(scores.size() == 3);
    assert(scores[2].numberValue == 92.0);
  }

  {
    const std::string json = R"({"text":"Line1\nLine2\tTabbed","unicode":"\u0041\u0042\u0043"})";
    JsonValue root = JsonParser(json).parse();
    auto& obj = *root.objectValue;
    assert(obj.at("text").stringValue == "Line1\nLine2\tTabbed");
    assert(obj.at("unicode").stringValue == "ABC");
  }

  {
    const std::string json = R"({"emoji":"\uD83D\uDE00"})";
    JsonValue root = JsonParser(json).parse();
    auto& obj = *root.objectValue;
    assert(obj.at("emoji").stringValue == "\xF0\x9F\x98\x80");
  }

  {
    const std::string json = "{\n  \"x\" : [1, 2, {\"y\":3}], \"z\": true\n}";
    JsonValue root = JsonParser(json).parse();
    auto& obj = *root.objectValue;
    assert(obj.at("z").boolValue == true);
    assert(obj.at("x").arrayValue->size() == 3);
  }

  // parser error handling
  expect_parse_failure(R"({"name":"John","age":})");
  expect_parse_failure(R"({"name":"John","age":30,})");
  expect_parse_failure("\"abc");              // unterminated string
  expect_parse_failure("\"\n\"");             // control char inside string
  expect_parse_failure(R"("\u12G4")");       // invalid hex
  expect_parse_failure(R"("\uD83D")");       // missing low surrogate
  expect_parse_failure(R"("\uDE00")");       // unexpected low surrogate
  expect_parse_failure(R"({"num":01})");      // invalid leading zero

  // fast validity check
  assert(json_fast_validity_check(R"({"id":"x"})"));
  assert(json_fast_validity_check(R"([1,2,3])"));
  assert(!json_fast_validity_check("abc"));
  assert(!json_fast_validity_check(R"({"a":1}{"b":2})"));
  assert(!json_fast_validity_check(R"({"a":[1,2})"));
  assert(!json_fast_validity_check(R"({"a":"unterminated})"));

  // key extraction
  assert(extract_json_string_value(R"({"id":"A"})", "id", "NULL") == "A");
  assert(extract_json_string_value(R"({"id" : "A"})", "id", "NULL") == "A");
  assert(extract_json_string_value(R"({"meta":{"id":"nested"},"id":"root"})", "id", "NULL") == "root");
  assert(extract_json_string_value(R"({"id":42})", "id", "NULL") == "NULL");
  assert(extract_json_string_value(R"({"x":"foo \"id\":\"B\"","id":"C"})", "id", "NULL") == "C");

  log_info("All tests for djson_parsing passed successfully.\n");
}

int main() {
  runTests();
  return 0;
}
