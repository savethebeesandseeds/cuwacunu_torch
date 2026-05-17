#include <camahjucunu/db/idydb.h>

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include <unistd.h>

namespace fs = std::filesystem;

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
  const uint64_t v = dist(gen);
  return std::to_string(static_cast<unsigned long long>(getpid())) + "_" +
         std::to_string(static_cast<unsigned long long>(v));
}

struct temp_dir_t {
  fs::path dir{};
  temp_dir_t() {
    dir = fs::temp_directory_path() / ("test_db_query_utils_" + random_suffix());
    fs::create_directories(dir);
  }
  ~temp_dir_t() {
    std::error_code ec;
    fs::remove_all(dir, ec);
  }
};

static void test_filters_and_null_checks(const fs::path& db_file) {
  idydb* db = nullptr;
  REQUIRE(idydb_open(db_file.c_str(), &db, IDYDB_CREATE) == IDYDB_SUCCESS);

  REQUIRE(idydb_insert_int(&db, 1, 1, 1) == IDYDB_DONE);
  REQUIRE(idydb_insert_int(&db, 1, 2, 2) == IDYDB_DONE);
  REQUIRE(idydb_insert_int(&db, 1, 3, 3) == IDYDB_DONE);
  REQUIRE(idydb_insert_int(&db, 1, 4, 4) == IDYDB_DONE);

  REQUIRE(idydb_insert_float(&db, 2, 1, 1.25f) == IDYDB_DONE);
  REQUIRE(idydb_insert_float(&db, 2, 2, 2.50f) == IDYDB_DONE);
  REQUIRE(idydb_insert_float(&db, 2, 3, -1.0f) == IDYDB_DONE);

  REQUIRE(idydb_insert_bool(&db, 3, 1, true) == IDYDB_DONE);
  REQUIRE(idydb_insert_bool(&db, 3, 2, false) == IDYDB_DONE);
  REQUIRE(idydb_insert_bool(&db, 3, 4, true) == IDYDB_DONE);

  REQUIRE(idydb_insert_const_char(&db, 4, 1, "alpha") == IDYDB_DONE);
  REQUIRE(idydb_insert_const_char(&db, 4, 2, "beta") == IDYDB_DONE);
  REQUIRE(idydb_insert_const_char(&db, 4, 3, "alpha") == IDYDB_DONE);

  REQUIRE(idydb_insert_const_char(&db, 5, 2, "nonnull") == IDYDB_DONE);

  {
    idydb_filter_term f{};
    f.column = 1;
    f.type = IDYDB_INTEGER;
    f.op = IDYDB_FILTER_OP_GT;
    f.value.i = 2;

    db::query::query_spec_t q{};
    q.filters = {f};
    q.select_columns = {1};

    std::vector<idydb_column_row_sizing> rows{};
    std::string error;
    REQUIRE(db::query::select_rows(&db, q, &rows, &error));
    REQUIRE(rows.size() == 2);
    REQUIRE(rows[0] == 3);
    REQUIRE(rows[1] == 4);
  }

  {
    idydb_filter_term f{};
    f.column = 2;
    f.type = IDYDB_FLOAT;
    f.op = IDYDB_FILTER_OP_LTE;
    f.value.f = 1.25f;

    db::query::query_spec_t q{};
    q.filters = {f};
    q.select_columns = {2};

    std::vector<idydb_column_row_sizing> rows{};
    std::string error;
    REQUIRE(db::query::select_rows(&db, q, &rows, &error));
    REQUIRE(rows.size() == 2);
    REQUIRE(rows[0] == 1);
    REQUIRE(rows[1] == 3);
  }

  {
    idydb_filter_term f{};
    f.column = 3;
    f.type = IDYDB_BOOL;
    f.op = IDYDB_FILTER_OP_EQ;
    f.value.b = true;

    std::size_t count = 0;
    std::string error;
    REQUIRE(db::query::count_rows(&db, {f}, &count, &error));
    REQUIRE(count == 2);
  }

  {
    idydb_filter_term f{};
    f.column = 4;
    f.type = IDYDB_CHAR;
    f.op = IDYDB_FILTER_OP_EQ;
    f.value.s = "alpha";

    bool exists = false;
    std::string error;
    REQUIRE(db::query::exists_row(&db, {f}, &exists, &error));
    REQUIRE(exists);
  }

  {
    idydb_filter_term f{};
    f.column = 5;
    f.type = IDYDB_NULL;
    f.op = IDYDB_FILTER_OP_IS_NULL;

    std::size_t count = 0;
    std::string error;
    REQUIRE(db::query::count_rows(&db, {f}, &count, &error));
    REQUIRE(count == 1);
  }

  REQUIRE(idydb_close(&db) == IDYDB_DONE);
}

static void test_limit_offset_ordering_and_latest(const fs::path& db_file) {
  idydb* db = nullptr;
  REQUIRE(idydb_open(db_file.c_str(), &db, IDYDB_CREATE) == IDYDB_SUCCESS);

  for (idydb_column_row_sizing row = 1; row <= 2048; ++row) {
    REQUIRE(idydb_insert_const_char(&db, 1, row, "k") == IDYDB_DONE);
    REQUIRE(idydb_insert_int(&db, 2, row, static_cast<int>(row)) == IDYDB_DONE);
  }

  {
    idydb_filter_term f{};
    f.column = 1;
    f.type = IDYDB_CHAR;
    f.op = IDYDB_FILTER_OP_EQ;
    f.value.s = "k";

    db::query::query_spec_t q{};
    q.filters = {f};
    q.select_columns = {1};
    q.offset = 10;
    q.limit = 5;

    std::vector<idydb_column_row_sizing> rows{};
    std::string error;
    REQUIRE(db::query::select_rows(&db, q, &rows, &error));
    REQUIRE(rows.size() == 5);
    REQUIRE(rows[0] == 11);
    REQUIRE(rows[4] == 15);
  }

  {
    idydb_filter_term f{};
    f.column = 1;
    f.type = IDYDB_CHAR;
    f.op = IDYDB_FILTER_OP_EQ;
    f.value.s = "k";

    db::query::query_spec_t q{};
    q.filters = {f};
    q.select_columns = {1};
    q.limit = 3;
    q.newest_first = true;

    std::vector<idydb_column_row_sizing> rows{};
    std::string error;
    REQUIRE(db::query::select_rows(&db, q, &rows, &error));
    REQUIRE(rows.size() == 3);
    REQUIRE(rows[0] == 2048);
    REQUIRE(rows[1] == 2047);
    REQUIRE(rows[2] == 2046);
  }

  REQUIRE(idydb_insert_const_char(&db, 3, 1, "dup") == IDYDB_DONE);
  REQUIRE(idydb_insert_const_char(&db, 3, 4, "dup") == IDYDB_DONE);
  REQUIRE(idydb_insert_const_char(&db, 3, 7, "dup") == IDYDB_DONE);

  {
    idydb_column_row_sizing row = 0;
    std::string error;
    REQUIRE(db::query::latest_row_by_key(&db, 3, "dup", &row, &error));
    REQUIRE(row == 7);
  }

  {
    std::vector<idydb_value> values{};
    std::string error;
    REQUIRE(db::query::fetch_row(&db, 11, {1, 2}, &values, &error));
    REQUIRE(values.size() == 2);
    REQUIRE(values[0].type == IDYDB_CHAR);
    REQUIRE(values[1].type == IDYDB_INTEGER);
    REQUIRE(values[1].as.i == 11);
    idydb_values_free(values.data(), values.size());
  }

  {
    std::vector<db::query::query_row_t> rows{};
    std::string error;
    REQUIRE(db::query::fetch_rows(&db, {2048, 1}, {2}, &rows, &error));
    REQUIRE(rows.size() == 2);
    REQUIRE(rows[0].row_id == 2048);
    REQUIRE(rows[0].values.size() == 1);
    REQUIRE(rows[0].values[0].type == IDYDB_INTEGER);
    REQUIRE(rows[0].values[0].as.i == 2048);
    for (auto& row : rows) {
      idydb_values_free(row.values.data(), row.values.size());
      row.values.clear();
    }
  }

  REQUIRE(idydb_close(&db) == IDYDB_DONE);
}

static void test_encrypted_queries(const fs::path& db_file) {
  constexpr const char* kPass = "query-utils-pass";

  idydb* db = nullptr;
  REQUIRE(idydb_open_encrypted(db_file.c_str(), &db, IDYDB_CREATE, kPass) ==
          IDYDB_SUCCESS);
  REQUIRE(idydb_insert_const_char(&db, 1, 1, "enc") == IDYDB_DONE);
  REQUIRE(idydb_insert_int(&db, 2, 1, 99) == IDYDB_DONE);
  REQUIRE(idydb_close(&db) == IDYDB_DONE);

  REQUIRE(idydb_open_encrypted(db_file.c_str(), &db, 0, kPass) == IDYDB_SUCCESS);
  {
    idydb_filter_term f{};
    f.column = 1;
    f.type = IDYDB_CHAR;
    f.op = IDYDB_FILTER_OP_EQ;
    f.value.s = "enc";

    db::query::query_spec_t q{};
    q.filters = {f};
    q.select_columns = {1};

    std::vector<idydb_column_row_sizing> rows{};
    std::string error;
    REQUIRE(db::query::select_rows(&db, q, &rows, &error));
    REQUIRE(rows.size() == 1);
    REQUIRE(rows.front() == 1);
  }
  REQUIRE(idydb_close(&db) == IDYDB_DONE);

  REQUIRE(idydb_open_encrypted(db_file.c_str(), &db, 0, "wrong-pass") !=
          IDYDB_SUCCESS);
  if (db) {
    (void)idydb_close(&db);
  }
}

static void test_wave_cursor_indexing_helpers() {
  const db::wave_cursor::layout_t layout{
      .run_bits = 24,
      .episode_bits = 20,
      .batch_bits = 19,
  };
  REQUIRE(db::wave_cursor::valid_layout(layout));

  const db::wave_cursor::parts_t p{
      .run_id = 0x12345ULL,
      .episode_k = 33ULL,
      .batch_j = 7ULL,
  };

  std::uint64_t packed = 0;
  REQUIRE(db::wave_cursor::pack(layout, p, &packed));

  db::wave_cursor::parts_t roundtrip{};
  REQUIRE(db::wave_cursor::unpack(layout, packed, &roundtrip));
  REQUIRE(roundtrip.run_id == p.run_id);
  REQUIRE(roundtrip.episode_k == p.episode_k);
  REQUIRE(roundtrip.batch_j == p.batch_j);

  db::wave_cursor::masked_query_t q{};
  REQUIRE(db::wave_cursor::build_masked_query(
      layout,
      p,
      static_cast<std::uint8_t>(db::wave_cursor::field_run |
                                db::wave_cursor::field_episode),
      &q));

  const db::wave_cursor::parts_t same_scope_diff_batch{
      .run_id = p.run_id,
      .episode_k = p.episode_k,
      .batch_j = p.batch_j + 1ULL,
  };
  std::uint64_t packed_same_scope = 0;
  REQUIRE(db::wave_cursor::pack(layout, same_scope_diff_batch, &packed_same_scope));
  REQUIRE(db::wave_cursor::match_masked(packed_same_scope, q));

  const db::wave_cursor::parts_t different_scope{
      .run_id = p.run_id + 1ULL,
      .episode_k = p.episode_k,
      .batch_j = p.batch_j,
  };
  std::uint64_t packed_different_scope = 0;
  REQUIRE(db::wave_cursor::pack(layout, different_scope, &packed_different_scope));
  REQUIRE(!db::wave_cursor::match_masked(packed_different_scope, q));

  // Overflow must fail (run_id exceeds 24 bits in this layout).
  const db::wave_cursor::parts_t overflow{
      .run_id = (1ULL << 24),
      .episode_k = 0ULL,
      .batch_j = 0ULL,
  };
  REQUIRE(!db::wave_cursor::pack(layout, overflow, &packed));
}

int main() {
  temp_dir_t tmp{};
  test_filters_and_null_checks(tmp.dir / "filters.idydb");
  test_limit_offset_ordering_and_latest(tmp.dir / "ordering.idydb");
  test_encrypted_queries(tmp.dir / "enc.idydb");
  test_wave_cursor_indexing_helpers();
  std::cout << "[PASS] test_db_query_utils\n";
  return 0;
}
