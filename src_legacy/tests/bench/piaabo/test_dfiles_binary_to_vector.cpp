#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <type_traits>
#include <vector>

#include "piaabo/dfiles.h"

namespace fs = std::filesystem;
namespace dfiles = cuwacunu::piaabo::dfiles;

struct sample_record_t {
  int32_t id;
  double value;

  static sample_record_t from_binary(const char* data_ptr) {
    sample_record_t out{};
    std::memcpy(&out, data_ptr, sizeof(sample_record_t));
    return out;
  }
};

static_assert(std::is_trivially_copyable<sample_record_t>::value,
              "sample_record_t must be trivially copyable");

static std::string make_tmp_path(const std::string& suffix) {
  static size_t counter = 0;
  ++counter;
  const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
  return (fs::temp_directory_path() /
          ("cuwacunu_test_dfiles_" + std::to_string(now) + "_" +
           std::to_string(counter) + suffix))
      .string();
}

template <typename F>
static void expect_runtime_error(F&& fn) {
  bool thrown = false;
  try {
    fn();
  } catch (const std::runtime_error&) {
    thrown = true;
  }
  assert(thrown);
}

template <typename T>
static void write_records(const std::string& path, const std::vector<T>& records) {
  std::ofstream out(path, std::ios::binary | std::ios::out | std::ios::trunc);
  assert(out.is_open());
  if (!records.empty()) {
    out.write(reinterpret_cast<const char*>(records.data()),
              static_cast<std::streamsize>(records.size() * sizeof(T)));
  }
  assert(out.good());
}

static void test_happy_path() {
  const std::string path = make_tmp_path(".bin");
  const std::vector<sample_record_t> input = {
      {1, 1.25}, {2, -3.0}, {3, 999.875}, {4, 0.0}};
  write_records(path, input);

  const std::vector<sample_record_t> output =
      dfiles::binaryFile_to_vector<sample_record_t>(path, 2);
  assert(output.size() == input.size());
  for (size_t i = 0; i < input.size(); ++i) {
    assert(output[i].id == input[i].id);
    assert(output[i].value == input[i].value);
  }

  fs::remove(path);
}

static void test_zero_buffer_rejected() {
  const std::string path = make_tmp_path(".bin");
  write_records(path, std::vector<sample_record_t>{{42, 1.0}});

  expect_runtime_error([&]() {
    (void)dfiles::binaryFile_to_vector<sample_record_t>(path, 0);
  });

  fs::remove(path);
}

static void test_non_multiple_file_size_rejected() {
  const std::string path = make_tmp_path(".bin");
  {
    std::ofstream out(path, std::ios::binary | std::ios::out | std::ios::trunc);
    assert(out.is_open());
    const char raw[3] = {'a', 'b', 'c'};
    out.write(raw, sizeof(raw));
    assert(out.good());
  }

  expect_runtime_error([&]() {
    (void)dfiles::binaryFile_to_vector<sample_record_t>(path, 1);
  });

  fs::remove(path);
}

static void test_missing_file_rejected() {
  const std::string path = make_tmp_path(".missing.bin");
  expect_runtime_error([&]() {
    (void)dfiles::binaryFile_to_vector<sample_record_t>(path, 4);
  });
}

int main() {
  test_happy_path();
  test_zero_buffer_rejected();
  test_non_multiple_file_size_rejected();
  test_missing_file_rejected();
  return 0;
}
