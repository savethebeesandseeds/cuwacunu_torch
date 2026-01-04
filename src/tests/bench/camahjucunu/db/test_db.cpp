// test_db.cpp (C++20)
// Compile example (adjust include/lib paths to your project):
// g++ -std=c++20 -O2 -Wall -Wextra -I../../../include test_db.cpp
//   ../../../impl/camahjucunu/db/db.cpp -L$(SSL_PATH)/lib/x86_64-linux-gnu/ -lcrypto -lssl -o test_db
//
// Run:
// ./test_db

#include <camahjucunu/db/db.h>

#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include <unistd.h> // getpid()

namespace fs = std::filesystem;
using namespace cuwacunu::camahjucunu::db;

static void require_impl(bool ok, const char* expr, const char* file, int line) {
  if (!ok) {
    std::cerr << "[TEST FAIL] " << file << ":" << line << "  (" << expr << ")\n";
    std::exit(1);
  }
}
#define REQUIRE(x) require_impl((x), #x, __FILE__, __LINE__)

static std::string random_suffix() {
  std::random_device rd;
  std::mt19937_64 gen(rd());
  std::uniform_int_distribution<uint64_t> dist;
  uint64_t v = dist(gen);
  return std::to_string((unsigned long long)getpid()) + "_" + std::to_string((unsigned long long)v);
}

struct TempDir {
  fs::path dir;
  explicit TempDir(std::string name_prefix) {
    fs::path base = fs::temp_directory_path();
    dir = base / (name_prefix + "_" + random_suffix());
    fs::create_directories(dir);
  }
  ~TempDir() {
    std::error_code ec;
    fs::remove_all(dir, ec);
  }
};

static fs::path pjoin(const fs::path& dir, const std::string& name) {
  return dir / name;
}

static bool file_starts_with_magic(const fs::path& p, const char* magic8) {
  std::ifstream in(p, std::ios::binary);
  if (!in) return false;
  char buf[8]{};
  in.read(buf, 8);
  if (in.gcount() != 8) return false;
  return std::memcmp(buf, magic8, 8) == 0;
}

static void safe_close_on_failed_open(idydb** db) {
  if (db && *db) {
    // Even if open failed midway, close frees handler & releases locks when possible.
    (void)idydb_close(db);
  }
}

static void test_plaintext_basic(const fs::path& dbfile) {
  std::cout << "[TEST] plaintext_basic\n";

  idydb* db = nullptr;
  int rc = idydb_open(dbfile.c_str(), &db, IDYDB_CREATE);
  REQUIRE(rc == IDYDB_SUCCESS);
  REQUIRE(db != nullptr);

  // Insert basic types into column 1 (rows 1..4)
  REQUIRE(idydb_insert_int(&db, 1, 1, 1337) == IDYDB_DONE);
  REQUIRE(idydb_insert_float(&db, 1, 2, 3.14159f) == IDYDB_DONE);
  REQUIRE(idydb_insert_const_char(&db, 1, 3, "hello world") == IDYDB_DONE);
  REQUIRE(idydb_insert_bool(&db, 1, 4, true) == IDYDB_DONE);

  // Extract + verify
  REQUIRE(idydb_extract(&db, 1, 1) == IDYDB_DONE);
  REQUIRE(idydb_retrieved_type(&db) == IDYDB_INTEGER);
  REQUIRE(idydb_retrieve_int(&db) == 1337);

  REQUIRE(idydb_extract(&db, 1, 2) == IDYDB_DONE);
  REQUIRE(idydb_retrieved_type(&db) == IDYDB_FLOAT);
  REQUIRE(std::fabs(idydb_retrieve_float(&db) - 3.14159f) < 1e-4f);

  REQUIRE(idydb_extract(&db, 1, 3) == IDYDB_DONE);
  REQUIRE(idydb_retrieved_type(&db) == IDYDB_CHAR);
  {
    const char* s = idydb_retrieve_char(&db);
    REQUIRE(s != nullptr);
    REQUIRE(std::string(s) == "hello world");
  }

  REQUIRE(idydb_extract(&db, 1, 4) == IDYDB_DONE);
  REQUIRE(idydb_retrieved_type(&db) == IDYDB_BOOL);
  REQUIRE(idydb_retrieve_bool(&db) == true);

  // Vector column 2 rows 1..2
  {
    float v1[4] = {1.f, 0.f, 0.f, 0.f};
    float v2[4] = {0.f, 1.f, 0.f, 0.f};
    REQUIRE(idydb_insert_vector(&db, 2, 1, v1, 4) == IDYDB_DONE);
    REQUIRE(idydb_insert_vector(&db, 2, 2, v2, 4) == IDYDB_DONE);

    REQUIRE(idydb_extract(&db, 2, 1) == IDYDB_DONE);
    REQUIRE(idydb_retrieved_type(&db) == IDYDB_VECTOR);
    unsigned short dims = 0;
    const float* got = idydb_retrieve_vector(&db, &dims);
    REQUIRE(got != nullptr);
    REQUIRE(dims == 4);
    for (int i = 0; i < 4; ++i) REQUIRE(std::fabs(got[i] - v1[i]) < 1e-6f);
  }

  // Delete and ensure null
  REQUIRE(idydb_delete(&db, 1, 3) == IDYDB_DONE);
  int erc = idydb_extract(&db, 1, 3);
  REQUIRE(erc == IDYDB_NULL); // expected: value not found

  // next-row utility
  // We wrote to column 1 rows {1,2,4} (and deleted row 3), so max row is 4, next should be 5.
  REQUIRE(idydb_column_next_row(&db, 1) == 5);

  REQUIRE(idydb_close(&db) == IDYDB_DONE);
}

static void test_encrypted_newfile(const fs::path& dbfile, const std::string& pass) {
  std::cout << "[TEST] encrypted_newfile\n";

  idydb* db = nullptr;
  int rc = idydb_open_encrypted(dbfile.c_str(), &db, IDYDB_CREATE, pass.c_str());
  REQUIRE(rc == IDYDB_SUCCESS);
  REQUIRE(db != nullptr);

  REQUIRE(idydb_insert_int(&db, 1, 1, 4242) == IDYDB_DONE);
  REQUIRE(idydb_insert_const_char(&db, 1, 2, "secret") == IDYDB_DONE);

  REQUIRE(idydb_close(&db) == IDYDB_DONE);

  // On-disk should now be encrypted
  REQUIRE(file_starts_with_magic(dbfile, "IDYDBENC"));

  // Reopen with correct passphrase
  db = nullptr;
  rc = idydb_open_encrypted(dbfile.c_str(), &db, 0, pass.c_str());
  REQUIRE(rc == IDYDB_SUCCESS);

  REQUIRE(idydb_extract(&db, 1, 1) == IDYDB_DONE);
  REQUIRE(idydb_retrieved_type(&db) == IDYDB_INTEGER);
  REQUIRE(idydb_retrieve_int(&db) == 4242);

  REQUIRE(idydb_extract(&db, 1, 2) == IDYDB_DONE);
  REQUIRE(idydb_retrieved_type(&db) == IDYDB_CHAR);
  REQUIRE(std::string(idydb_retrieve_char(&db)) == "secret");

  REQUIRE(idydb_close(&db) == IDYDB_DONE);

  // Wrong passphrase should fail
  db = nullptr;
  rc = idydb_open_encrypted(dbfile.c_str(), &db, 0, "wrong-passphrase");
  REQUIRE(rc != IDYDB_SUCCESS);
  safe_close_on_failed_open(&db);
}

static void test_plaintext_to_encrypted_migration(const fs::path& dbfile, const std::string& pass) {
  std::cout << "[TEST] plaintext_to_encrypted_migration\n";

  // Create plaintext DB
  {
    idydb* db = nullptr;
    int rc = idydb_open(dbfile.c_str(), &db, IDYDB_CREATE);
    REQUIRE(rc == IDYDB_SUCCESS);

    REQUIRE(idydb_insert_int(&db, 1, 1, 7) == IDYDB_DONE);
    REQUIRE(idydb_insert_const_char(&db, 2, 1, "migrate-me") == IDYDB_DONE);

    REQUIRE(idydb_close(&db) == IDYDB_DONE);
  }

  // File should NOT start with IDYDBENC yet
  REQUIRE(!file_starts_with_magic(dbfile, "IDYDBENC"));

  // Open encrypted on plaintext -> migration mode (will encrypt on close if writable)
  {
    idydb* db = nullptr;
    int rc = idydb_open_encrypted(dbfile.c_str(), &db, 0, pass.c_str());
    REQUIRE(rc == IDYDB_SUCCESS);

    // Confirm we can read the old plaintext content
    REQUIRE(idydb_extract(&db, 1, 1) == IDYDB_DONE);
    REQUIRE(idydb_retrieved_type(&db) == IDYDB_INTEGER);
    REQUIRE(idydb_retrieve_int(&db) == 7);

    REQUIRE(idydb_extract(&db, 2, 1) == IDYDB_DONE);
    REQUIRE(idydb_retrieved_type(&db) == IDYDB_CHAR);
    REQUIRE(std::string(idydb_retrieve_char(&db)) == "migrate-me");

    REQUIRE(idydb_close(&db) == IDYDB_DONE);
  }

  // After close, backing should be encrypted
  REQUIRE(file_starts_with_magic(dbfile, "IDYDBENC"));

  // Reopen encrypted and confirm data persists
  {
    idydb* db = nullptr;
    int rc = idydb_open_encrypted(dbfile.c_str(), &db, 0, pass.c_str());
    REQUIRE(rc == IDYDB_SUCCESS);

    REQUIRE(idydb_extract(&db, 1, 1) == IDYDB_DONE);
    REQUIRE(idydb_retrieved_type(&db) == IDYDB_INTEGER);
    REQUIRE(idydb_retrieve_int(&db) == 7);

    REQUIRE(idydb_extract(&db, 2, 1) == IDYDB_DONE);
    REQUIRE(idydb_retrieved_type(&db) == IDYDB_CHAR);
    REQUIRE(std::string(idydb_retrieve_char(&db)) == "migrate-me");

    REQUIRE(idydb_close(&db) == IDYDB_DONE);
  }
}

static void test_readonly_behavior_encrypted(const fs::path& dbfile, const std::string& pass) {
  std::cout << "[TEST] readonly_behavior_encrypted\n";

  // Create encrypted DB with one value
  {
    idydb* db = nullptr;
    int rc = idydb_open_encrypted(dbfile.c_str(), &db, IDYDB_CREATE, pass.c_str());
    REQUIRE(rc == IDYDB_SUCCESS);
    REQUIRE(idydb_insert_int(&db, 1, 1, 1) == IDYDB_DONE);
    REQUIRE(idydb_close(&db) == IDYDB_DONE);
  }

  // Open readonly and ensure inserts are blocked
  {
    idydb* db = nullptr;
    int rc = idydb_open_encrypted(dbfile.c_str(), &db, IDYDB_READONLY, pass.c_str());
    REQUIRE(rc == IDYDB_SUCCESS);

    int irc = idydb_insert_int(&db, 1, 2, 2);
    REQUIRE(irc == IDYDB_READONLY);

    REQUIRE(idydb_extract(&db, 1, 1) == IDYDB_DONE);
    REQUIRE(idydb_retrieve_int(&db) == 1);

    REQUIRE(idydb_close(&db) == IDYDB_DONE);
  }
}

static void test_vectors_knn_and_rag(const fs::path& dbfile) {
  std::cout << "[TEST] vectors_knn_and_rag\n";

  idydb* db = nullptr;
  int rc = idydb_open(dbfile.c_str(), &db, IDYDB_CREATE);
  REQUIRE(rc == IDYDB_SUCCESS);

  // RAG pattern: text column 10, vector column 11
  constexpr idydb_column_row_sizing TEXT_COL = 10;
  constexpr idydb_column_row_sizing VEC_COL  = 11;

  // Insert 3 rows
  {
    float e1[2] = {1.f, 0.f};
    float e2[2] = {0.f, 1.f};
    float e3[2] = {0.9f, 0.1f};

    REQUIRE(idydb_rag_upsert_text(&db, TEXT_COL, VEC_COL, 1, "alpha", e1, 2) == IDYDB_DONE);
    REQUIRE(idydb_rag_upsert_text(&db, TEXT_COL, VEC_COL, 2, "beta",  e2, 2) == IDYDB_DONE);
    REQUIRE(idydb_rag_upsert_text(&db, TEXT_COL, VEC_COL, 3, "gamma", e3, 2) == IDYDB_DONE);
  }

  // Check next row helper for text column
  REQUIRE(idydb_column_next_row(&db, TEXT_COL) == 4);

  // Direct kNN on vector column
  {
    float q[2] = {1.f, 0.f};
    idydb_knn_result res[2]{};
    int n = idydb_knn_search_vector_column(&db, VEC_COL, q, 2, 2, IDYDB_SIM_COSINE, res);
    REQUIRE(n == 2);
    REQUIRE(res[0].row == 1); // alpha should be best match
    REQUIRE(res[0].score >= res[1].score);
  }

  // RAG topk query returning texts
  {
    float q[2] = {1.f, 0.f};
    constexpr unsigned short K = 2;
    idydb_knn_result res[K]{};
    char* out_texts[K]{nullptr, nullptr};

    int n = idydb_rag_query_topk(&db, TEXT_COL, VEC_COL, q, 2, K, IDYDB_SIM_COSINE, res, out_texts);
    REQUIRE(n == 2);
    REQUIRE(out_texts[0] != nullptr);
    REQUIRE(std::string(out_texts[0]) == "alpha");

    for (int i = 0; i < n; ++i) {
      if (out_texts[i]) std::free(out_texts[i]);
    }
  }

  // RAG context builder
  {
    float q[2] = {1.f, 0.f};
    char* ctx = nullptr;
    int rc2 = idydb_rag_query_context(&db, TEXT_COL, VEC_COL, q, 2, 2, IDYDB_SIM_COSINE, 0, &ctx);
    REQUIRE(rc2 == IDYDB_DONE);
    REQUIRE(ctx != nullptr);

    std::string s(ctx);
    REQUIRE(s.find("alpha") != std::string::npos);
    // second result should likely be gamma
    REQUIRE(s.find("gamma") != std::string::npos);

    std::free(ctx);
  }

  REQUIRE(idydb_close(&db) == IDYDB_DONE);
}

int main() {
  TempDir tdir("idydb_cpp20_tests");

  const std::string passphrase = "correct horse battery staple";

  // 1) Plaintext basics
  test_plaintext_basic(pjoin(tdir.dir, "plain.db"));

  // 2) Encrypted new file + reopen + wrong pass
  test_encrypted_newfile(pjoin(tdir.dir, "enc_new.db"), passphrase);

  // 3) Plaintext -> encrypted migration
  test_plaintext_to_encrypted_migration(pjoin(tdir.dir, "migrate.db"), passphrase);

  // 4) Readonly behavior on encrypted DB
  test_readonly_behavior_encrypted(pjoin(tdir.dir, "enc_ro.db"), passphrase);

  // 5) Vectors + kNN + RAG
  test_vectors_knn_and_rag(pjoin(tdir.dir, "rag.db"));

  std::cout << "[ALL TESTS PASSED]\n";
  return 0;
}
