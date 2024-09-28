#include "piaabo/dutils.h"
#include "piaabo/json_parsing.h"

using namespace cuwacunu;
using namespace piaabo;

// Test suite function
void runTests() {
  // Test 1: Simple JSON object
  {
    std::string jsonString = R"({"name": "John", "age": 25})";
    JsonParser parser(jsonString);
    JsonValue root = parser.parse();

    assert(root.type == JsonValueType::OBJECT);
    assert(root.objectValue->size() == 2);

    auto& obj = *root.objectValue;
    assert(obj["name"].type == JsonValueType::STRING);
    assert(obj["name"].stringValue == "John");
    assert(obj["age"].type == JsonValueType::NUMBER);
    assert(obj["age"].numberValue == 25);
  }

  // Test 2: Nested objects and arrays
  {
    std::string jsonString = R"({
      "person": {
        "name": "Alice",
        "age": 30,
        "isStudent": false,
        "scores": [85, 90, 92]
      }
    })";
    JsonParser parser(jsonString);
    JsonValue root = parser.parse();

    assert(root.type == JsonValueType::OBJECT);
    auto& obj = *root.objectValue;
    assert(obj["person"].type == JsonValueType::OBJECT);
    auto& person = *obj["person"].objectValue;
    assert(person["name"].stringValue == "Alice");
    assert(person["age"].numberValue == 30);
    assert(person["isStudent"].boolValue == false);
    assert(person["scores"].type == JsonValueType::ARRAY);
    auto& scores = *person["scores"].arrayValue;
    assert(scores.size() == 3);
    assert(scores[0].numberValue == 85);
    assert(scores[1].numberValue == 90);
    assert(scores[2].numberValue == 92);
  }

  // Test 3: Empty object and array
  {
    std::string jsonString = R"({"emptyObject": {}, "emptyArray": []})";
    JsonParser parser(jsonString);
    JsonValue root = parser.parse();

    auto& obj = *root.objectValue;
    assert(obj["emptyObject"].type == JsonValueType::OBJECT);
    assert(obj["emptyObject"].objectValue->empty());
    assert(obj["emptyArray"].type == JsonValueType::ARRAY);
    assert(obj["emptyArray"].arrayValue->empty());
  }

  // Test 4: String with escape characters
  {
    std::string jsonString = R"({"text": "Line1\nLine2\tTabbed"})";
    JsonParser parser(jsonString);
    JsonValue root = parser.parse();

    auto& obj = *root.objectValue;
    assert(obj["text"].type == JsonValueType::STRING);
    assert(obj["text"].stringValue == "Line1\nLine2\tTabbed");
  }

  // Test 5: Numbers with fractional and exponential parts
  {
    std::string jsonString = R"({"int": 42, "float": 3.14, "exp": 1e10, "negExp": -2.5E-3})";
    JsonParser parser(jsonString);
    JsonValue root = parser.parse();

    auto& obj = *root.objectValue;
    assert(obj["int"].numberValue == 42);
    assert(obj["float"].numberValue == 3.14);
    assert(obj["exp"].numberValue == 1e10);
    assert(obj["negExp"].numberValue == -2.5e-3);
  }

  // Test 6: Boolean and null values
  {
    std::string jsonString = R"({"trueVal": true, "falseVal": false, "nullVal": null})";
    JsonParser parser(jsonString);
    JsonValue root = parser.parse();

    auto& obj = *root.objectValue;
    assert(obj["trueVal"].type == JsonValueType::BOOLEAN);
    assert(obj["trueVal"].boolValue == true);
    assert(obj["falseVal"].type == JsonValueType::BOOLEAN);
    assert(obj["falseVal"].boolValue == false);
    assert(obj["nullVal"].type == JsonValueType::NULL_TYPE);
  }

  // Test 7: Unicode characters in strings
  {
    std::string jsonString = R"({"unicode": "\u0041\u0042\u0043"})";
    JsonParser parser(jsonString);
    JsonValue root = parser.parse();

    auto& obj = *root.objectValue;
    assert(obj["unicode"].type == JsonValueType::STRING);
    assert(obj["unicode"].stringValue == "ABC");
  }

  // Test 8: Invalid JSON (should throw an exception)
  {
    std::string jsonString = R"({"name": "John", "age": })";
    try {
      JsonParser parser(jsonString);
      JsonValue root = parser.parse();
      assert(false);  // Should not reach here
    } catch (const std::runtime_error& e) {
      std::cout << e.what() << std::endl;
      assert(std::string(e.what()).find("Invalid value") != std::string::npos ||
        std::string(e.what()).find("Runtime error occurred") != std::string::npos);
    }
  }

  // Test 9: Trailing commas (not allowed in standard JSON)
  {
    std::string jsonString = R"({"name": "John", "age": 30,})";
    try {
      JsonParser parser(jsonString);
      JsonValue root = parser.parse();
      assert(false);  // Should not reach here
    } catch (const std::runtime_error& e) {
      assert(std::string(e.what()).find("Expected '}'") != std::string::npos ||
        std::string(e.what()).find("Expected '\"'") != std::string::npos ||
        std::string(e.what()).find("Runtime error occurred") != std::string::npos);
    }
  }

  // Test 10: Whitespace handling
  {
    std::string jsonString = "{ \n\t\"name\" : \t\"Jane\" \n}";
    JsonParser parser(jsonString);
    JsonValue root = parser.parse();

    auto& obj = *root.objectValue;
    assert(obj["name"].stringValue == "Jane");
  }

  // Test 11: Large JSON
  {
    std::string jsonString = R"({
      "users": [
        {"id": 1, "name": "User1"},
        {"id": 2, "name": "User2"},
        {"id": 3, "name": "User3"},
        {"id": 4, "name": "User4"},
        {"id": 5, "name": "User5"}
      ]
    })";
    JsonParser parser(jsonString);
    JsonValue root = parser.parse();

    auto& users = *root.objectValue->at("users").arrayValue;
    assert(users.size() == 5);
    for (int i = 0; i < 5; ++i) {
      auto& user = *users[i].objectValue;
      assert(user["id"].numberValue == i + 1);
      assert(user["name"].stringValue == "User" + std::to_string(i + 1));
    }
  }

  // Test 12: Empty JSON
  {
    std::string jsonString = R"({})";
    JsonParser parser(jsonString);
    JsonValue root = parser.parse();

    assert(root.type == JsonValueType::OBJECT);
    assert(root.objectValue->empty());
  }

  // All tests passed
  log_info("All tests for json_parsing.h passed successfully.\n");
}

int main() {
  runTests();
  return 0;
}
