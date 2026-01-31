// test_db_rag.cpp (C++20)
//
// RAG end-to-end test for IdyDB vector DB + RAG extensions.
// - Uses idydb_set_embedder + idydb_rag_upsert_text_auto_embed (typical RAG workflow)
// - Tests kNN (cosine + L2), dim filtering, upsert updates, deletes, context builder truncation
// - Tests persistence across reopen
// - Tests READONLY behavior (writes blocked, reads/query still OK)
//
// IMPORTANT: This test keeps all created database files on disk (no cleanup).
// Artifacts default to: ./idydb_test_artifacts/<runid>/
// Override base dir with env var: IDYDB_TEST_OUTDIR=/path/to/dir
//
// Compile example (adjust include/lib paths to your project):
// g++ -std=c++20 -O2 -Wall -Wextra -I../../../include test_db_rag.cpp 
//     ../../../impl/camahjucunu/db/db.cpp -L$(SSL_PATH)/lib/x86_64-linux-gnu/ -lcrypto -lssl 
//     -o test_db_rag
//
// Run:
// ./test_db_rag

#include <camahjucunu/db/db.h>

#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include <unistd.h> // getpid()

namespace fs = std::filesystem;
using namespace cuwacunu::camahjucunu::db;

// ---------------- Minimal test harness ----------------

static void require_impl(bool ok, const char* expr, const char* file, int line) {
  if (!ok) {
    std::cerr << "[TEST FAIL] " << file << ":" << line << "  (" << expr << ")\n";
    std::exit(1);
  }
}
#define REQUIRE(x) require_impl((x), #x, __FILE__, __LINE__)

// ---------------- Utilities ----------------

static std::string random_suffix() {
  std::random_device rd;
  std::mt19937_64 gen(rd());
  std::uniform_int_distribution<uint64_t> dist;
  uint64_t v = dist(gen);
  return std::to_string((unsigned long long)getpid()) + "_" + std::to_string((unsigned long long)v);
}

static fs::path make_artifacts_dir(const std::string& prefix) {
  const char* env = std::getenv("IDYDB_TEST_OUTDIR");
  fs::path base = env ? fs::path(env) : (fs::current_path() / "idydb_test_artifacts");

  std::error_code ec;
  fs::create_directories(base, ec);

  fs::path dir = base / (prefix + "_" + random_suffix());
  fs::create_directories(dir, ec);

  return dir;
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
    (void)idydb_close(db);
  }
}

static int open_db_rag(const fs::path& file,
                       idydb** db,
                       bool encrypted,
                       const std::string& passphrase,
                       int flags)
{
  if (!encrypted) return idydb_open(file.c_str(), db, flags);
  return idydb_open_encrypted(file.c_str(), db, flags, passphrase.c_str());
}

// ---------------- Deterministic embedder ----------------
//
// Token-hash bag-of-words into 32 dims, then L2 normalize.
// Allocates output with calloc() so caller can free() it.

static int fnv1a_token_embedder(const char* text,
                               float** out_vector,
                               unsigned short* out_dims,
                               void* user)
{
  (void)user;
  if (!text || !out_vector || !out_dims) return -1;

  constexpr unsigned short D = 32;
  float* v = static_cast<float*>(std::calloc(D, sizeof(float)));
  if (!v) return -1;

  constexpr uint64_t FNV_OFFSET = 1469598103934665603ULL;
  constexpr uint64_t FNV_PRIME  = 1099511628211ULL;

  auto is_tok = [](unsigned char c) -> bool { return std::isalnum(c) != 0; };

  size_t i = 0;
  while (text[i] != '\0') {
    while (text[i] != '\0' && !is_tok(static_cast<unsigned char>(text[i]))) ++i;
    if (text[i] == '\0') break;

    uint64_t h = FNV_OFFSET;
    while (text[i] != '\0' && is_tok(static_cast<unsigned char>(text[i]))) {
      unsigned char c = static_cast<unsigned char>(text[i]);
      c = static_cast<unsigned char>(std::tolower(c));
      h ^= static_cast<uint64_t>(c);
      h *= FNV_PRIME;
      ++i;
    }

    unsigned idx = static_cast<unsigned>(h % D);
    v[idx] += 1.0f;
  }

  // L2 normalize
  float ss = 0.0f;
  for (unsigned short k = 0; k < D; ++k) ss += v[k] * v[k];
  if (ss > 0.0f) {
    float inv = 1.0f / std::sqrt(ss);
    for (unsigned short k = 0; k < D; ++k) v[k] *= inv;
  }

  *out_vector = v;
  *out_dims = D;
  return 0;
}

static std::vector<float> embed_for_query(const std::string& s) {
  float* vec = nullptr;
  unsigned short dims = 0;
  int rc = fnv1a_token_embedder(s.c_str(), &vec, &dims, nullptr);
  REQUIRE(rc == 0);
  REQUIRE(vec != nullptr);
  REQUIRE(dims == 32);

  std::vector<float> out(vec, vec + dims);
  std::free(vec);
  return out;
}

// ---------------- The RAG workflow test ----------------

static void test_rag_workflow_end_to_end(const fs::path& dbfile,
                                        bool encrypted,
                                        const std::string& passphrase)
{
  std::cout << "[TEST] rag_workflow_end_to_end" << (encrypted ? "_enc" : "_plain") << "\n";
  std::cout << "       dbfile = " << dbfile << "\n";

  idydb* db = nullptr;
  int rc = open_db_rag(dbfile, &db, encrypted, passphrase, IDYDB_CREATE);
  REQUIRE(rc == IDYDB_SUCCESS);
  REQUIRE(db != nullptr);

  // Typical RAG layout: separate text and embedding columns
  constexpr idydb_column_row_sizing TEXT_COL = 20;
  constexpr idydb_column_row_sizing VEC_COL  = 21;

  // Set deterministic embedder (runtime)
  idydb_set_embedder(&db, &fnv1a_token_embedder, nullptr);

  const std::string DOC1 = "the quick brown fox";
  const std::string DOC3 = "the quick blue hare";
  const std::string DOC4 = "fox in the woods";
  const std::string DOC5 = "lorem ipsum dolor sit amet";

  // Insert docs using AUTO-EMBED path (typical RAG ingestion)
  REQUIRE(idydb_rag_upsert_text_auto_embed(&db, TEXT_COL, VEC_COL, 1, DOC1.c_str()) == IDYDB_DONE);
  REQUIRE(idydb_rag_upsert_text_auto_embed(&db, TEXT_COL, VEC_COL, 2, DOC1.c_str()) == IDYDB_DONE); // duplicate on purpose
  REQUIRE(idydb_rag_upsert_text_auto_embed(&db, TEXT_COL, VEC_COL, 3, DOC3.c_str()) == IDYDB_DONE);
  REQUIRE(idydb_rag_upsert_text_auto_embed(&db, TEXT_COL, VEC_COL, 4, DOC4.c_str()) == IDYDB_DONE);
  REQUIRE(idydb_rag_upsert_text_auto_embed(&db, TEXT_COL, VEC_COL, 5, DOC5.c_str()) == IDYDB_DONE);

  // Insert a mixed-dims embedding into the SAME vector column.
  // Search should ignore mismatched dims.
  {
    float bad3[3] = {1.f, 2.f, 3.f};
    REQUIRE(idydb_insert_const_char(&db, TEXT_COL, 6, "dims=3 row") == IDYDB_DONE);
    REQUIRE(idydb_insert_vector(&db, VEC_COL, 6, bad3, 3) == IDYDB_DONE);
  }

  // Next-row helper: max row=6 => next=7
  REQUIRE(idydb_column_next_row(&db, TEXT_COL) == 7);
  REQUIRE(idydb_column_next_row(&db, VEC_COL) == 7);

  // ---- kNN sanity: dims filter works (dims=32 should not return row 6) ----
  {
    auto q1 = embed_for_query(DOC1);

    idydb_knn_result res10[10]{};
    int n = idydb_knn_search_vector_column(&db, VEC_COL,
                                          q1.data(), (unsigned short)q1.size(),
                                          10, IDYDB_SIM_COSINE, res10);
    REQUIRE(n == 5); // rows 1..5 only
    for (int i = 0; i < n; ++i) REQUIRE(res10[i].row != 6);
  }

  // ---- dims=3 query should match ONLY row 6 with exact L2 score ~ 0 ----
  {
    float q3[3] = {1.f, 2.f, 3.f};
    idydb_knn_result best{};
    int n = idydb_knn_search_vector_column(&db, VEC_COL, q3, 3, 1, IDYDB_SIM_L2, &best);
    REQUIRE(n == 1);
    REQUIRE(best.row == 6);
    REQUIRE(std::fabs(best.score) < 1e-6f); // identical => -0
  }

  // ---- Deterministic exact-match selection (k=1, L2, duplicates) ----
  // DOC1 exists at rows 1 and 2 with identical embeddings. With k=1, L2 picks the first exact match (row 1).
  auto q_doc1 = embed_for_query(DOC1);
  {
    idydb_knn_result best{};
    int n = idydb_knn_search_vector_column(&db, VEC_COL,
                                          q_doc1.data(), (unsigned short)q_doc1.size(),
                                          1, IDYDB_SIM_L2, &best);
    REQUIRE(n == 1);
    REQUIRE(best.row == 1);
    REQUIRE(std::fabs(best.score) < 1e-6f);
  }

  // ---- Upsert update changes ranking deterministically ----
  // Update row 1 to unrelated text; row 2 remains exact DOC1 match => best becomes row 2.
  {
    const char* NEW1 = "slow green turtle";
    REQUIRE(idydb_rag_upsert_text_auto_embed(&db, TEXT_COL, VEC_COL, 1, NEW1) == IDYDB_DONE);

    REQUIRE(idydb_extract(&db, TEXT_COL, 1) == IDYDB_DONE);
    REQUIRE(idydb_retrieved_type(&db) == IDYDB_CHAR);
    REQUIRE(std::string(idydb_retrieve_char(&db)) == NEW1);

    idydb_knn_result best{};
    int n = idydb_knn_search_vector_column(&db, VEC_COL,
                                          q_doc1.data(), (unsigned short)q_doc1.size(),
                                          1, IDYDB_SIM_L2, &best);
    REQUIRE(n == 1);
    REQUIRE(best.row == 2);
    REQUIRE(std::fabs(best.score) < 1e-6f);
  }

  // ---- Delete removes from both text + vector columns ----
  {
    REQUIRE(idydb_delete(&db, TEXT_COL, 2) == IDYDB_DONE);
    REQUIRE(idydb_delete(&db, VEC_COL, 2) == IDYDB_DONE);

    int erc = idydb_extract(&db, TEXT_COL, 2);
    REQUIRE(erc == IDYDB_NULL);

    // Remaining dims=32 vectors: rows {1,3,4,5} => 4 vectors
    idydb_knn_result res10[10]{};
    int n = idydb_knn_search_vector_column(&db, VEC_COL,
                                          q_doc1.data(), (unsigned short)q_doc1.size(),
                                          10, IDYDB_SIM_COSINE, res10);
    REQUIRE(n == 4);
    for (int i = 0; i < n; ++i) REQUIRE(res10[i].row != 2);
  }

  // ---- RAG topk returns texts (end-to-end) ----
  {
    auto q_doc4 = embed_for_query(DOC4);

    idydb_knn_result r1[1]{};
    char* t1[1]{nullptr};

    int n = idydb_rag_query_topk(&db, TEXT_COL, VEC_COL,
                                q_doc4.data(), (unsigned short)q_doc4.size(),
                                1, IDYDB_SIM_L2, r1, t1);
    REQUIRE(n == 1);
    REQUIRE(r1[0].row == 4);
    REQUIRE(t1[0] != nullptr);
    REQUIRE(std::string(t1[0]) == DOC4);
    std::free(t1[0]);
  }

  // ---- RAG context builder with truncation ----
  {
    auto q_doc4 = embed_for_query(DOC4);

    char* ctx = nullptr;
    int rc2 = idydb_rag_query_context(&db, TEXT_COL, VEC_COL,
                                     q_doc4.data(), (unsigned short)q_doc4.size(),
                                     3, IDYDB_SIM_COSINE,
                                     24, // intentionally small
                                     &ctx);
    REQUIRE(rc2 == IDYDB_DONE);
    REQUIRE(ctx != nullptr);

    REQUIRE(std::strlen(ctx) <= 24);
    std::string s(ctx);
    REQUIRE(s.find("fox") != std::string::npos);

    std::free(ctx);
  }

  // ---- Wrong dims should produce 0 results ----
  {
    auto q = embed_for_query(DOC4);
    q.pop_back(); // dims=31 (no stored vectors of this dim)

    idydb_knn_result res[3]{};
    char* texts[3]{nullptr, nullptr, nullptr};

    int n = idydb_rag_query_topk(&db, TEXT_COL, VEC_COL,
                                q.data(), (unsigned short)q.size(),
                                3, IDYDB_SIM_COSINE, res, texts);
    REQUIRE(n == 0);
    for (auto& p : texts) if (p) std::free(p);
  }

  // Close + check encrypted-at-rest magic if applicable
  REQUIRE(idydb_close(&db) == IDYDB_DONE);

  if (encrypted) {
    REQUIRE(file_starts_with_magic(dbfile, "IDYDBENC"));
  } else {
    // plaintext db should generally NOT start with the encrypted container magic
    REQUIRE(!file_starts_with_magic(dbfile, "IDYDBENC"));
  }

  // Reopen and verify persistence + querying still works
  db = nullptr;
  rc = open_db_rag(dbfile, &db, encrypted, passphrase, 0);
  REQUIRE(rc == IDYDB_SUCCESS);
  REQUIRE(db != nullptr);

  idydb_set_embedder(&db, &fnv1a_token_embedder, nullptr);

  {
    auto q_doc4 = embed_for_query(DOC4);
    idydb_knn_result best{};
    int n = idydb_knn_search_vector_column(&db, VEC_COL,
                                          q_doc4.data(), (unsigned short)q_doc4.size(),
                                          1, IDYDB_SIM_L2, &best);
    REQUIRE(n == 1);
    REQUIRE(best.row == 4);
    REQUIRE(std::fabs(best.score) < 1e-6f);
  }

  REQUIRE(idydb_close(&db) == IDYDB_DONE);

  // Reopen READONLY: upsert must fail, queries must succeed
  db = nullptr;
  rc = open_db_rag(dbfile, &db, encrypted, passphrase, IDYDB_READONLY);
  REQUIRE(rc == IDYDB_SUCCESS);
  REQUIRE(db != nullptr);

  idydb_set_embedder(&db, &fnv1a_token_embedder, nullptr);

  {
    int urc = idydb_rag_upsert_text_auto_embed(&db, TEXT_COL, VEC_COL, 7, "should fail in readonly");
    REQUIRE(urc == IDYDB_READONLY);
  }

  {
    auto q_doc4 = embed_for_query(DOC4);
    idydb_knn_result best{};
    int n = idydb_knn_search_vector_column(&db, VEC_COL,
                                          q_doc4.data(), (unsigned short)q_doc4.size(),
                                          1, IDYDB_SIM_L2, &best);
    REQUIRE(n == 1);
    REQUIRE(best.row == 4);
  }

  REQUIRE(idydb_close(&db) == IDYDB_DONE);
}

// ---------------- main ----------------

int main() {
  const std::string passphrase = "correct horse battery staple";

  // Keep artifacts (no deletion)
  fs::path outdir = make_artifacts_dir("idydb_rag_tests");
  std::cout << "[INFO] Keeping DB artifacts under:\n  " << outdir << "\n";

  const fs::path plain_db = outdir / "rag_workflow_plain.db";
  const fs::path enc_db   = outdir / "rag_workflow_enc.db";

  test_rag_workflow_end_to_end(plain_db, false, passphrase);
  test_rag_workflow_end_to_end(enc_db,   true,  passphrase);

  std::cout << "[ALL TESTS PASSED]\n";
  std::cout << "[INFO] DB files preserved:\n";
  std::cout << "  " << plain_db << "\n";
  std::cout << "  " << enc_db << "\n";
  return 0;
}
